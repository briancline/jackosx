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
	CAAudioBufferList.cp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "CAAudioBufferList.h"
#include "CADebugMacros.h"
#include "CALogMacros.h"
#include <stdlib.h>
#include <string.h>

//=============================================================================
//	CAAudioBufferList
//=============================================================================

AudioBufferList*	CAAudioBufferList::Create(UInt32 inNumberBuffers)
{
	AudioBufferList* theAnswer = static_cast<AudioBufferList*>(calloc(1, sizeof(UInt32) + (inNumberBuffers * sizeof(AudioBuffer))));
	if(theAnswer != NULL)
	{
		theAnswer->mNumberBuffers = inNumberBuffers;
	}
	return theAnswer;
}

void	CAAudioBufferList::Destroy(AudioBufferList* inBufferList)
{
	free(inBufferList);
}

UInt32	CAAudioBufferList::GetTotalNumberChannels(const AudioBufferList& inBufferList)
{
	UInt32 theAnswer = 0;
	
	for(UInt32 theIndex = 0; theIndex < inBufferList.mNumberBuffers; ++theIndex)
	{
		theAnswer += inBufferList.mBuffers[theIndex].mNumberChannels;
	}
	
	return theAnswer;
}

bool	CAAudioBufferList::GetBufferForChannel(const AudioBufferList& inBufferList, UInt32 inChannel, UInt32& outBufferNumber, UInt32& outBufferChannel)
{
	bool theAnswer = false;
	UInt32 theIndex = 0;
	
	while((theIndex < inBufferList.mNumberBuffers) && (inChannel > inBufferList.mBuffers[theIndex].mNumberChannels))
	{
		++theIndex;
		inChannel -= inBufferList.mBuffers[theIndex].mNumberChannels;
	}
	
	if(theIndex < inBufferList.mNumberBuffers)
	{
		outBufferNumber = theIndex;
		outBufferChannel = inChannel;
		theAnswer = true;
	}
	
	return theAnswer;
}

void	CAAudioBufferList::Clear(AudioBufferList& outBufferList)
{
	//	assumes that "0" is actually the 0 value for this stream format
	for(UInt32 theBufferIndex = 0; theBufferIndex < outBufferList.mNumberBuffers; ++theBufferIndex)
	{
		if(outBufferList.mBuffers[theBufferIndex].mData != NULL)
		{
			memset(outBufferList.mBuffers[theBufferIndex].mData, 0, outBufferList.mBuffers[theBufferIndex].mDataByteSize);
		}
	}
}

void	CAAudioBufferList::Copy(const AudioBufferList& inBufferList, AudioBufferList& outBufferList, UInt32 inStartingOutputChannel)
{
	UInt32 theInputChannel = 0;
	UInt32 theNumberInputChannels = GetTotalNumberChannels(inBufferList);
	UInt32 theOutputChannel = inStartingOutputChannel;
	UInt32 theNumberOutputChannels = GetTotalNumberChannels(outBufferList);
	
	while((theInputChannel < theNumberInputChannels) && (theOutputChannel < theNumberOutputChannels))
	{
		UInt32 theInputBufferIndex;
		UInt32 theInputBufferChannel;
		GetBufferForChannel(inBufferList, theInputChannel, theInputBufferIndex, theInputBufferChannel);
		UInt32 theInputBufferFrameSize = inBufferList.mBuffers[theInputBufferIndex].mDataByteSize / sizeof(Float32);
		Float32* theInputBuffer = static_cast<Float32*>(inBufferList.mBuffers[theInputBufferIndex].mData);
		
		UInt32 theOutputBufferIndex;
		UInt32 theOutputBufferChannel;
		GetBufferForChannel(inBufferList, theOutputChannel, theOutputBufferIndex, theOutputBufferChannel);
		UInt32 theOutputBufferFrameSize = outBufferList.mBuffers[theOutputBufferIndex].mDataByteSize / sizeof(Float32);
		Float32* theOutputBuffer = static_cast<Float32*>(outBufferList.mBuffers[theOutputBufferIndex].mData);
		
		UInt32 theFrameIndex = 0;
		UInt32 theInputBufferFrame = theInputBufferChannel;
		UInt32 theOutputBufferFrame = theOutputBufferChannel;
		while((theFrameIndex < theInputBufferFrameSize) && (theFrameIndex < theOutputBufferFrameSize))
		{
			theOutputBuffer[theOutputBufferFrame] = theInputBuffer[theInputBufferFrame];
			++theFrameIndex;
			theInputBufferFrame += inBufferList.mBuffers[theInputBufferIndex].mNumberChannels;
			theOutputBufferFrame += outBufferList.mBuffers[theOutputBufferIndex].mNumberChannels;
		}
		
		++theInputChannel;
		++theOutputChannel;
	}
}

void	CAAudioBufferList::Sum(const AudioBufferList& inSourceBufferList, AudioBufferList& ioSummedBufferList)
{
	//	assumes that the buffers are Float32 samples and the listst have the same layout
	//	this is a lame algorithm, by the way. it could at least be unrolled a couple of times
	for(UInt32 theBufferIndex = 0; theBufferIndex < ioSummedBufferList.mNumberBuffers; ++theBufferIndex)
	{
		Float32* theSourceBuffer = static_cast<Float32*>(inSourceBufferList.mBuffers[theBufferIndex].mData);
		Float32* theSummedBuffer = static_cast<Float32*>(ioSummedBufferList.mBuffers[theBufferIndex].mData);
		UInt32 theNumberSamplesToMix = ioSummedBufferList.mBuffers[theBufferIndex].mDataByteSize / sizeof(Float32);
		if((theSourceBuffer != NULL) && (theSummedBuffer != NULL) && (theNumberSamplesToMix > 0))
		{
			while(theNumberSamplesToMix > 0)
			{
				*theSummedBuffer += *theSourceBuffer;
				++theSummedBuffer;
				++theSourceBuffer;
				--theNumberSamplesToMix;
			}
		}
	}
}

#if CoreAudio_Debug
void	CAAudioBufferList::PrintToLog(const AudioBufferList& inBufferList)
{
	PrintInt("  Number streams: ", inBufferList.mNumberBuffers);
	
	for(UInt32 theIndex = 0; theIndex < inBufferList.mNumberBuffers; ++theIndex)
	{
		PrintIndexedInt("  Channels in stream", theIndex + 1, inBufferList.mBuffers[theIndex].mNumberChannels);
		PrintIndexedInt("  Buffer size of stream", theIndex + 1, inBufferList.mBuffers[theIndex].mDataByteSize);
	}
}
#endif
