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
	CAAudioStreamBasicDesc.cp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "CAAudioStreamBasicDesc.h"
#include "CADebugMacros.h"
#include "CALogMacros.h"

//=============================================================================
//	CAAudioStreamBasicDescription
//=============================================================================

CAAudioStreamBasicDescription::CAAudioStreamBasicDescription()
{
	mSampleRate = 0;
	mFormatID = 0;
	mBytesPerPacket = 0;
	mFramesPerPacket = 0;
	mBytesPerFrame = 0;
	mChannelsPerFrame = 0;
	mBitsPerChannel = 0;
	mFormatFlags = 0;
}

CAAudioStreamBasicDescription::CAAudioStreamBasicDescription(const AudioStreamBasicDescription& v)
{
	mSampleRate = v.mSampleRate;
	mFormatID = v.mFormatID;
	mBytesPerPacket = v.mBytesPerPacket;
	mFramesPerPacket = v.mFramesPerPacket;
	mBytesPerFrame = v.mBytesPerFrame;
	mChannelsPerFrame = v.mChannelsPerFrame;
	mBitsPerChannel = v.mBitsPerChannel;
	mFormatFlags = v.mFormatFlags;
}

CAAudioStreamBasicDescription::CAAudioStreamBasicDescription(double inSampleRate, UInt32 inFormatID, UInt32 inBytesPerPacket, UInt32 inFramesPerPacket, UInt32 inBytesPerFrame, UInt32 inChannelsPerFrame, UInt32 inBitsPerChannel, UInt32 inFormatFlags)
{
	mSampleRate = inSampleRate;
	mFormatID = inFormatID;
	mBytesPerPacket = inBytesPerPacket;
	mFramesPerPacket = inFramesPerPacket;
	mBytesPerFrame = inBytesPerFrame;
	mChannelsPerFrame = inChannelsPerFrame;
	mBitsPerChannel = inBitsPerChannel;
	mFormatFlags = inFormatFlags;
}

CAAudioStreamBasicDescription&	CAAudioStreamBasicDescription::operator=(const AudioStreamBasicDescription& v)
{
	mSampleRate = v.mSampleRate;
	mFormatID = v.mFormatID;
	mBytesPerPacket = v.mBytesPerPacket;
	mFramesPerPacket = v.mFramesPerPacket;
	mBytesPerFrame = v.mBytesPerFrame;
	mChannelsPerFrame = v.mChannelsPerFrame;
	mBitsPerChannel = v.mBitsPerChannel;
	mFormatFlags = v.mFormatFlags;
	return *this;
}

void	CAAudioStreamBasicDescription::NormalizeLinearPCMFormat(AudioStreamBasicDescription& ioDescription)
{
	if(ioDescription.mFormatID == kAudioFormatLinearPCM)
	{
		//	we have to doctor the linear PCM format because we always
		//	only export 32 bit, native endian, fully packed floats
		ioDescription.mBitsPerChannel = sizeof(Float32) * 8;
		ioDescription.mBytesPerFrame = sizeof(Float32) * ioDescription.mChannelsPerFrame;
		ioDescription.mFramesPerPacket = 1;
		ioDescription.mBytesPerPacket = ioDescription.mFramesPerPacket * ioDescription.mBytesPerFrame;
		
		ioDescription.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
		ioDescription.mFormatFlags |= kAudioFormatFlagsNativeEndian;
	}
}

void	CAAudioStreamBasicDescription::ResetFormat(AudioStreamBasicDescription& ioDescription)
{
	ioDescription.mSampleRate = 0;
	ioDescription.mFormatID = 0;
	ioDescription.mBytesPerPacket = 0;
	ioDescription.mFramesPerPacket = 0;
	ioDescription.mBytesPerFrame = 0;
	ioDescription.mChannelsPerFrame = 0;
	ioDescription.mBitsPerChannel = 0;
	ioDescription.mFormatFlags = 0;
}

#if CoreAudio_Debug
void	CAAudioStreamBasicDescription::PrintToLog(const AudioStreamBasicDescription& inDesc)
{
	PrintFloat		("  Sample Rate:        ", inDesc.mSampleRate);
	Print4CharCode	("  Format ID:          ", inDesc.mFormatID);
	PrintHex		("  Format Flags:       ", inDesc.mFormatFlags);
	PrintInt		("  Bytes per Packet:   ", inDesc.mBytesPerPacket);
	PrintInt		("  Frames per Packet:  ", inDesc.mFramesPerPacket);
	PrintInt		("  Bytes per Frame:    ", inDesc.mBytesPerFrame);
	PrintInt		("  Channels per Frame: ", inDesc.mChannelsPerFrame);
	PrintInt		("  Bits per Channel:   ", inDesc.mBitsPerChannel);
}
#endif

bool	operator<(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	bool theAnswer = false;
	bool isDone = false;
	
	//	note that if either side is 0, that field is skipped
	
	//	format ID is the first order sort
	if((!isDone) && ((x.mFormatID != 0) && (y.mFormatID != 0)))
	{
		if(x.mFormatID != y.mFormatID)
		{
			//	formats are sorted numerically except that linear
			//	PCM is always first
			if(x.mFormatID == kAudioFormatLinearPCM)
			{
				theAnswer = true;
			}
			else if(y.mFormatID == kAudioFormatLinearPCM)
			{
				theAnswer = false;
			}
			else
			{
				theAnswer = x.mFormatID < y.mFormatID;
			}
			isDone = true;
		}
	}
	
	//	floating point vs integer for linear PCM only
	if((!isDone) && ((x.mFormatID == kAudioFormatLinearPCM) && (y.mFormatID == kAudioFormatLinearPCM)) && ((x.mFormatFlags != 0) && (y.mFormatFlags != 0)))
	{
		if((x.mFormatFlags & kAudioFormatFlagIsFloat) != (y.mFormatFlags & kAudioFormatFlagIsFloat))
		{
			//	floating point is better than integer
			theAnswer = y.mFormatFlags & kAudioFormatFlagIsFloat;
			isDone = true;
		}
	}
	
	//	bit depth
	if((!isDone) && ((x.mBitsPerChannel != 0) && (y.mBitsPerChannel != 0)))
	{
		if(x.mBitsPerChannel != y.mBitsPerChannel)
		{
			//	deeper bit depths are higher quality
			theAnswer = x.mBitsPerChannel < y.mBitsPerChannel;
			isDone = true;
		}
	}
	
	//	sample rate
	if((!isDone) && ((x.mSampleRate != 0) && (y.mSampleRate != 0)))
	{
		if(x.mSampleRate != y.mSampleRate)
		{
			//	higher sample rates are higher quality
			theAnswer = x.mSampleRate < y.mSampleRate;
			isDone = true;
		}
	}
	
	//	number of channels
	if((!isDone) && ((x.mChannelsPerFrame != 0) && (y.mChannelsPerFrame != 0)))
	{
		if(x.mChannelsPerFrame != y.mChannelsPerFrame)
		{
			//	more channels is higher quality
			theAnswer = x.mChannelsPerFrame < y.mChannelsPerFrame;
			isDone = true;
		}
	}
	
	return theAnswer;
}

bool	operator==(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	//	the semantics for equality are:
	//		1) Values must match exactly
	//		2) 0's are ignored in the comparison
	
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
