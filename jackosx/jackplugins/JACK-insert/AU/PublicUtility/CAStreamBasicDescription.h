/*	Copyright: 	© Copyright 2002 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
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
	CAStreamBasicDescription.h
	
=============================================================================*/

#ifndef __CAStreamBasicDescription_h__
#define __CAStreamBasicDescription_h__

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudioTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void CAShowStreamDescription(const AudioStreamBasicDescription *fmt);

#ifdef __cplusplus
};
#endif

// ____________________________________________________________________________
//
//	CAStreamBasicDescription
class CAStreamBasicDescription : public AudioStreamBasicDescription {
public:
	CAStreamBasicDescription() { }
	
	CAStreamBasicDescription(const AudioStreamBasicDescription &desc)
	{
		SetFrom(desc);
	}
	
	void	SetFrom(const AudioStreamBasicDescription &desc)
	{
		memcpy(this, &desc, sizeof(AudioStreamBasicDescription));
	}
	
	// _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
	//
	// interrogation
	
	bool	IsPCM() const { return mFormatID == kAudioFormatLinearPCM; }
	
	bool	PackednessIsSignificant() const
	{
		check(IsPCM());
		return (SampleWordSize() << 3) != mBitsPerChannel;
	}
	
	bool	AlignmentIsSignificant() const
	{
		return PackednessIsSignificant() || (mBitsPerChannel & 7) != 0;
	}
	
	bool	IsInterleaved() const
	{
		return !IsPCM() || !(mFormatFlags & kAudioFormatFlagIsNonInterleaved);
	}
	
	// for sanity with interleaved/deinterleaved possibilities, never access mChannelsPerFrame, use these:
	UInt32	NumberInterleavedChannels() const	{ return IsInterleaved() ? mChannelsPerFrame : 1; }	
	UInt32	NumberChannelStreams() const		{ return IsInterleaved() ? 1 : mChannelsPerFrame; }
	UInt32	NumberChannels() const				{ return mChannelsPerFrame; }
	UInt32	SampleWordSize() const				{ return (mBytesPerFrame > 0) ? mBytesPerFrame / NumberInterleavedChannels() :  0;}

	UInt32	FramesToBytes(UInt32 nframes) const	{ return nframes * mBytesPerFrame; }
	UInt32	BytesToFrames(UInt32 nbytes) const	{ check(mBytesPerFrame > 0); return nbytes / mBytesPerFrame; }
	
	bool	SameChannelsAndInterleaving(const CAStreamBasicDescription &a) const
	{
		return this->NumberChannels() == a.NumberChannels() && this->IsInterleaved() == a.IsInterleaved();
	}
	
	// _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
	//
	//	manipulation
	
	void	SetCanonical(UInt32 nChannels, bool interleaved)
				// note: leaves sample rate untouched
	{
		mFormatID = kAudioFormatLinearPCM;
		mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
		mBitsPerChannel = 32;
		mChannelsPerFrame = nChannels;
		mFramesPerPacket = 1;
		if (interleaved)
			mBytesPerPacket = mBytesPerFrame = nChannels * sizeof(Float32);
		else {
			mBytesPerPacket = mBytesPerFrame = sizeof(Float32);
			mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
		}
	}
	
	void	ChangeNumberChannels(UInt32 nChannels, bool interleaved)
				// alter an existing format
	{
		check(IsPCM());
		UInt32 wordSize = SampleWordSize();	// get this before changing ANYTHING
		mChannelsPerFrame = nChannels;
		mFramesPerPacket = 1;
		if (interleaved) {
			mBytesPerPacket = mBytesPerFrame = nChannels * wordSize;
			mFormatFlags &= ~kAudioFormatFlagIsNonInterleaved;
		} else {
			mBytesPerPacket = mBytesPerFrame = wordSize;
			mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
		}
	}
	
	
	// _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
	//
	//	other
	
	void	Print() const
	{
		CAShowStreamDescription(this);
	}

	bool	operator==(const AudioStreamBasicDescription& y) const
	{
		//	the semantics for equality are:
		//		1) Values must match exactly
		//		2) 0's are ignored in the comparison
		const CAStreamBasicDescription &x = *this;
		
		//	assume they are
		bool	isEqual = true;
		
		//	check the sample rate
		if(isEqual)
		{
			if((x.mSampleRate != 0) && (y.mSampleRate != 0))
			{
				isEqual = x.mSampleRate == y.mSampleRate;
			}
		}
		
		//	check the format ID
		if(isEqual)
		{
			if((x.mFormatID != 0) && (y.mFormatID != 0))
			{
				isEqual = x.mFormatID == y.mFormatID;
			}
		}
		
		//	check the bytes per packet
		if(isEqual)
		{
			if((x.mBytesPerPacket != 0) && (y.mBytesPerPacket != 0))
			{
				isEqual = x.mBytesPerPacket == y.mBytesPerPacket;
			}
		}
		
		//	check the frames per packet
		if(isEqual)
		{
			if((x.mFramesPerPacket != 0) && (y.mFramesPerPacket != 0))
			{
				isEqual = x.mFramesPerPacket == y.mFramesPerPacket;
			}
		}
		
		//	check the bytes per frame
		if(isEqual)
		{
			if((x.mBytesPerFrame != 0) && (y.mBytesPerFrame != 0))
			{
				isEqual = x.mBytesPerFrame == y.mBytesPerFrame;
			}
		}
		
		//	check the channels per frame
		if(isEqual)
		{
			if((x.mChannelsPerFrame != 0) && (y.mChannelsPerFrame != 0))
			{
				isEqual = x.mChannelsPerFrame == y.mChannelsPerFrame;
			}
		}
		
		//	check the bits per channel
		if(isEqual)
		{
			if((x.mBitsPerChannel != 0) && (y.mBitsPerChannel != 0))
			{
				isEqual = x.mBitsPerChannel == y.mBitsPerChannel;
			}
		}
		
		//	check the format flags
		if(isEqual)
		{
			if((x.mFormatFlags != 0) && (y.mFormatFlags != 0))
			{
				isEqual = x.mFormatFlags == y.mFormatFlags;
			}
		}
		
		return isEqual;
	}

	bool	operator!=(const AudioStreamBasicDescription& y) const
	{
		return !(*this == y);
	}
};


#endif // __CAStreamBasicDescription_h__
