/*	Copyright: 	© Copyright 2002 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
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
	CABufferList.h
	
=============================================================================*/

#ifndef __CABufferList_h__
#define __CABufferList_h__

#include "CAStreamBasicDescription.h"

// ____________________________________________________________________________
//
//	CABufferList - variable length buffer list
//	Can optionally allocate memory to the buffers.
// 	All buffers are assumed to have the same format (number of channels, word size), so that
//		we can assume their mDataByteSizes are all the same.
class CABufferList {
public:
	void *	operator new(size_t size, int nBuffers) {
				return ::operator new(sizeof(CABufferList) + (nBuffers-1) * sizeof(AudioBuffer));
			}
	static CABufferList *	New(const char *name, const CAStreamBasicDescription &format)
	{
		UInt32 numBuffers = format.NumberChannelStreams(), channelsPerBuffer = format.NumberInterleavedChannels();
		return new(numBuffers) CABufferList(name, numBuffers, channelsPerBuffer);
	}

protected:
	CABufferList(const char *name, UInt32 numBuffers, UInt32 channelsPerBuffer) :
		mName(name),
		mExternallyOwnedBufferMemory(false),
		mBufferMemory(NULL)
	{
		check(numBuffers > 0 && channelsPerBuffer > 0);
		mNumberBuffers = numBuffers;
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf) {
			buf->mNumberChannels = channelsPerBuffer;
			buf->mDataByteSize = 0;
			buf->mData = NULL;
		}
	}

public:
	~CABufferList()
	{
		if (mBufferMemory && !mExternallyOwnedBufferMemory)
			delete[] mBufferMemory;
	}
	
	const char *				Name() { return mName; }
	const AudioBufferList &		GetBufferList() const { return *(AudioBufferList *)&mNumberBuffers; }
	AudioBufferList &			GetModifiableBufferList()
	{
		VerifyNotTrashingOwnedBuffer();
		return GetBufferList();
	}
	
	UInt32		GetNumBytes() const
	{
		return mBuffers[0].mDataByteSize;
	}
	
	void		SetBytes(UInt32 nBytes, void *data)
	{
		VerifyNotTrashingOwnedBuffer();
		check(mNumberBuffers == 1);
		mBuffers[0].mDataByteSize = nBytes;
		mBuffers[0].mData = data;
	}
	
	void		CopyAllFrom(CABufferList *srcbl, CABufferList *ptrbl)
					// copies bytes from srcbl
					// make ptrbl reflect the length copied
					// note that srcbl may be same as ptrbl!
	{
		// Note that this buffer *can* own memory and its pointers/lengths are not
		// altered; only its buffer contents, which are copied from srcbl.
		// The pointers/lengths in ptrbl are updated to reflect the addresses/lengths
		// of the copied data, and srcbl's contents are consumed.
		ptrbl->VerifyNotTrashingOwnedBuffer();
		UInt32 nBytes = srcbl->GetNumBytes();
		AudioBuffer *mybuf = mBuffers, *srcbuf = srcbl->mBuffers,
					*ptrbuf = ptrbl->mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++mybuf, ++srcbuf, ++ptrbuf) {
			memmove(mybuf->mData, srcbuf->mData, srcbuf->mDataByteSize);
			ptrbuf->mData = mybuf->mData;
			ptrbuf->mDataByteSize = srcbuf->mDataByteSize;
		}
		if (srcbl != ptrbl)
			srcbl->BytesConsumed(nBytes);
	}
	
	void		AppendFrom(CABufferList *blp, UInt32 nBytes)
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *mybuf = mBuffers, *srcbuf = blp->mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++mybuf, ++srcbuf) {
			check(nBytes <= srcbuf->mDataByteSize);
			memcpy((Byte *)mybuf->mData + mybuf->mDataByteSize, srcbuf->mData, nBytes);
			mybuf->mDataByteSize += nBytes;
		}
		blp->BytesConsumed(nBytes);
	}
	
	void		PadWithZeroes(UInt32 desiredBufferSize)
					// for cases where an algorithm (e.g. SRC) requires some
					// padding to create silence following end-of-file
	{
		VerifyNotTrashingOwnedBuffer();
		if (GetNumBytes() > desiredBufferSize) return;
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf) {
			memset((Byte *)buf->mData + buf->mDataByteSize, 0, desiredBufferSize - buf->mDataByteSize);
			buf->mDataByteSize = desiredBufferSize;
		}
	}
	
	void		SetToZeroes(UInt32 nBytes)
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf) {
			memset((Byte *)buf->mData, 0, nBytes);
			buf->mDataByteSize = nBytes;
		}
	}
	
	void		BytesConsumed(UInt32 nBytes)
					// advance buffer pointers, decrease buffer sizes
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf) {
			check(nBytes <= buf->mDataByteSize);
			buf->mData = (Byte *)buf->mData + nBytes;
			buf->mDataByteSize -= nBytes;
		}
	}
	
	void		SetFrom(const AudioBufferList *abl)
	{
		VerifyNotTrashingOwnedBuffer();
		memcpy(&GetBufferList(), abl, offsetof(AudioBufferList, mBuffers[abl->mNumberBuffers]));
	}
	
	void		SetFrom(const CABufferList *blp)
	{
		SetFrom(&blp->GetBufferList());
	}
	
	void		SetFrom(const AudioBufferList *abl, UInt32 nBytes)
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *mybuf = mBuffers;
		const AudioBuffer *srcbuf = abl->mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++mybuf, ++srcbuf) {
			mybuf->mNumberChannels = srcbuf->mNumberChannels;
			mybuf->mDataByteSize = nBytes;
			mybuf->mData = srcbuf->mData;
		}
	}
	
	void		SetFrom(const CABufferList *blp, UInt32 nBytes)
	{
		SetFrom(&blp->GetBufferList(), nBytes);
	}
	
	AudioBufferList *	ToAudioBufferList(AudioBufferList *abl) const
	{
		memcpy(abl, &GetBufferList(), offsetof(AudioBufferList, mBuffers[mNumberBuffers]));
		return abl;
	}
	
	void		AllocateBuffers(UInt32 nBytes);
	
	void		DeallocateBuffers();
	
	void		UseExternalBuffer(Byte *ptr, UInt32 nBytes);
	
	void		AdvanceBufferPointers(UInt32 nBytes)
					// this is for bufferlists that function simply as
					// an array of pointers into another bufferlist, being advanced,
					// as in RenderOutput implementations
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf)
			buf->mData = (Byte *)buf->mData + nBytes;
	}
	
	void		SetNumBytes(UInt32 nBytes)
	{
		VerifyNotTrashingOwnedBuffer();
		AudioBuffer *buf = mBuffers;
		for (UInt32 i = mNumberBuffers; i--; ++buf)
			buf->mDataByteSize = nBytes;
	}

#if DEBUG
	void		Print() const
	{
		printf("CABufferList 0x%lX \"%s\", owned memory 0x%X\n", long(this), mName, int(mBufferMemory));
		const AudioBuffer *buf = mBuffers;
		for (UInt32 i = 0; i < mNumberBuffers; ++i, ++buf)
			printf("  mBuffer[%ld]: ch=%ld, ptr=0x%lX, nbytes=0x%lX\n", 
				i, buf->mNumberChannels, long(buf->mData), buf->mDataByteSize);
	}
#endif

protected:
	AudioBufferList &	GetBufferList() { return *(AudioBufferList *)&mNumberBuffers; }	// use with care
							// if we make this public, then we lose ability to call VerifyNotTrashingOwnedBuffer
	void				VerifyNotTrashingOwnedBuffer()
	{
		// This needs to be called from places where we are modifying the buffer list.
		// It's an error to modify the buffer pointers or lengths if we own the buffer memory.
		check(mBufferMemory == NULL);
	}

	const char *						mName;	// for debugging
	bool								mExternallyOwnedBufferMemory;
	Byte *								mBufferMemory;
public:
	UInt32								mNumberBuffers;
	AudioBuffer							mBuffers[1];
};

#endif // __CABufferList_h__
