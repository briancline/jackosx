/*	Copyright: 	� Copyright 2002 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	FormatConverterClient.h
	
=============================================================================*/

#ifndef __FormatConverterClient_h__
#define __FormatConverterClient_h__

#include <AudioToolbox/AudioConverter.h>

extern "C" void CAShow(void *);

// ____________________________________________________________________________
//
// FormatConverterClient
// C++ wrapper for an AudioConverter
class FormatConverterClient {
public:
	FormatConverterClient() :
		mConverter(NULL)
	{
	}
	
		// ONLY NEEDED FOR Puma build - should be removed....
	FormatConverterClient(const AudioStreamBasicDescription &src, const AudioStreamBasicDescription &dest) :
		mConverter(NULL)
	{
		Initialize(src, dest);
	}
	
	virtual ~FormatConverterClient()
	{
		Destroy();
	}
	
	virtual OSStatus	Initialize(const AudioStreamBasicDescription &src, const AudioStreamBasicDescription &dest)
	{
		OSStatus err;
		if (mConverter)
			verify_noerr(AudioConverterDispose(mConverter));
		verify_noerr(err = AudioConverterNew(&src, &dest, &mConverter));
		mInputFormat = src;
		mOutputFormat = dest;
		return err;
	}
	
	void	Destroy()
	{
		if (mConverter) {
			verify_noerr(AudioConverterDispose(mConverter));
			mConverter = NULL;
		}
	}
	

	OSStatus	Reset () const { return (mConverter ? AudioConverterReset (mConverter) : noErr); }

	OSStatus	SetProperty(AudioConverterPropertyID	inPropertyID,
							UInt32						inPropertyDataSize,
							void*						inPropertyData)
	{
		return AudioConverterSetProperty(mConverter, inPropertyID, inPropertyDataSize, inPropertyData);
	}
	
	OSStatus	GetProperty(AudioConverterPropertyID	inPropertyID,
							UInt32 &					ioPropertyDataSize,
							void *						outPropertyData)
	{
		return AudioConverterGetProperty(mConverter, inPropertyID, &ioPropertyDataSize, outPropertyData);
	}

#if 0	
	OSStatus	ConvertBuffer(void *src, UInt32 srcSize, void *dest, UInt32 destSize)
	{
		OSStatus err;
		verify_noerr(err = AudioConverterConvertBuffer(mConverter, srcSize, src, &destSize, dest));
		return err;
	}
	
	OSStatus	FillBuffer(	UInt32&							ioOutputDataSize,
							void*							outOutputData)
	{
		OSStatus err;
		verify_noerr(err = AudioConverterFillBuffer(mConverter, InputProc, this, &ioOutputDataSize, outOutputData));
		return err;
	}
#endif

	OSStatus	FillComplexBuffer(	UInt32 &							ioOutputDataPacketSize,
									AudioBufferList &					outOutputData,
									AudioStreamPacketDescription*		outPacketDescription)
	{
		OSStatus err;
		err = AudioConverterFillComplexBuffer(mConverter, InputProc, this,
			&ioOutputDataPacketSize, &outOutputData, outPacketDescription);
//printf("\n\nFillComplexBuffer returned %ld packets\n", ioOutputDataPacketSize);
//DumpBufferList(outOutputData);
		return err;
	}
	
	const AudioStreamBasicDescription &	GetInputFormat()	{ return mInputFormat; }

	const AudioStreamBasicDescription &	GetOutputFormat()	{ return mOutputFormat; }	
	
protected:
	virtual OSStatus	FormatConverterInputProc(	
								UInt32 &						ioNumberDataPackets,
								AudioBufferList &				ioData,
								AudioStreamPacketDescription**	outDataPacketDescription) = 0;
	
private:
	static OSStatus	InputProc(	AudioConverterRef				inAudioConverter,
								UInt32*							ioNumberDataPackets,
								AudioBufferList*				ioData,
								AudioStreamPacketDescription**	outDataPacketDescription,
								void*							inUserData)
	{
		OSStatus err;
		try {
			FormatConverterClient *This = (FormatConverterClient *)inUserData;
			AudioBuffer *buf = ioData->mBuffers;
			UInt32 nBytes = *ioNumberDataPackets * This->mInputFormat.mBytesPerPacket;
			for (UInt32 i = ioData->mNumberBuffers; i--; ++buf) {
				buf->mDataByteSize = nBytes;
			}
//printf("\n\nInputProc - %ld packets, %ld bytes\n", *ioNumberDataPackets, nBytes);
			err = This->FormatConverterInputProc(*ioNumberDataPackets, *ioData, outDataPacketDescription);
//DumpBufferList(*ioData);
		} catch (OSStatus err) {
			return err;
		}
		catch (...) {
			return -1;
		}
		return err;
	}

#if DEBUG
public:
	void Show()
	{
		CAShow(mConverter);
	}

	static void	DumpBufferList(AudioBufferList &abl)
	{
		printf("%ld buffers:\n", abl.mNumberBuffers);
		AudioBuffer *buf = abl.mBuffers;
		for (UInt32 i = 0; i < abl.mNumberBuffers; ++i, ++buf) {
			printf("buffer[%ld]:\n", i);
			DumpBuffer(buf);
		}
	}
	
	static void	DumpBuffer(AudioBuffer *buf)
	{
		printf("  channels=%ld, ptr=0x%X, size=%ld\n", buf->mNumberChannels, int(buf->mData), buf->mDataByteSize);
		Float32 *p = static_cast<Float32 *>(buf->mData);
		for (int i = buf->mDataByteSize / sizeof(Float32); i--; ++p)
			printf("%f\n", *p);
	}
#endif

protected:
	AudioConverterRef			mConverter;
	AudioStreamBasicDescription	mInputFormat, mOutputFormat;
};

#endif // __FormatConverterClient_h__
