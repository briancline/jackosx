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
	CAGuard.cp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "CAGuard.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAHostTimeBase.h"

#if CoreAudio_Debug
//	#define	Log_Ownership		1
//	#define	Log_WaitOwnership	1
//	#define	Log_TimedWaits		1
//	#define Log_Latency			1
#endif

//#warning		Need a try-based Locker too
//=============================================================================
//	CAGuard
//=============================================================================

CAGuard::CAGuard(const char* inName)
:
	mName(inName),
	mOwner(0)
#if	Log_Average_Latency
	,mAverageLatencyAccumulator(0.0),
	mAverageLatencyCount(0)
#endif
{
	OSStatus theError = pthread_mutex_init(&mMutex, NULL);
	ThrowIf(theError != 0, CAException(theError), "CAGuard::CAGuard: Could not init the mutex");
	
	theError = pthread_cond_init(&mCondVar, NULL);
	ThrowIf(theError != 0, CAException(theError), "CAGuard::CAGuard: Could not init the cond var");
	
	mOwner = 0;
	
	#if	Log_Ownership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::CAGuard: creating %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), mName, mOwner);
	#endif
}

CAGuard::~CAGuard()
{
	#if	Log_Ownership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::~CAGuard: destroying %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), mName, mOwner);
	#endif
	pthread_mutex_destroy(&mMutex);
	pthread_cond_destroy(&mCondVar);
}

bool	CAGuard::Lock()
{
	bool theAnswer = false;
	
	if(pthread_self() != mOwner)
	{
		#if	Log_Ownership
			DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Lock: thread %p is locking %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
		#endif

		OSStatus theError = pthread_mutex_lock(&mMutex);
		ThrowIf(theError != 0, CAException(theError), "CAGuard::Lock: Could not lock the mutex");
		mOwner = pthread_self();
		theAnswer = true;
	
		#if	Log_Ownership
			DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Lock: thread %p has locked %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
		#endif
	}

	return theAnswer;
}

void	CAGuard::Unlock()
{
	if(pthread_self() == mOwner)
	{
		#if	Log_Ownership
			DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Unlock: thread %p is unlocking %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
		#endif

		mOwner = 0;
		OSStatus theError = pthread_mutex_unlock(&mMutex);
		ThrowIf(theError != 0, CAException(theError), "CAGuard::Unlock: Could not unlock the mutex");
	
		#if	Log_Ownership
			DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Unlock: thread %p has unlocked %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
		#endif
	}
	else
	{
		DebugMessage("CAGuard::Unlock: A thread is attempting to unlock a guard it doesn't own");
	}
}

bool	CAGuard::Try (bool& outWasLocked)
{
	bool theAnswer = false;
	outWasLocked = false;
	
	if((pthread_self() == mOwner) || (mOwner == NULL))
	{
		theAnswer = true;
		outWasLocked = Lock();
	}
	
	return theAnswer;
}

void	CAGuard::Wait()
{
	ThrowIf(pthread_self() != mOwner, CAException(1), "CAGuard::Wait: A thread has to have locked a guard be for it can wait");

	mOwner = 0;

	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Wait: thread %p is waiting on %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif

	OSStatus theError = pthread_cond_wait(&mCondVar, &mMutex);
	ThrowIf(theError != 0, CAException(theError), "CAGuard::Wait: Could not wait for a signal");
	mOwner = pthread_self();

	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Wait: thread %p waited on %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif
}

bool	CAGuard::WaitFor(UInt64 inNanos)
{
	ThrowIf(pthread_self() != mOwner, CAException(1), "CAGuard::WaitFor: A thread has to have locked a guard be for it can wait");

	#if	Log_TimedWaits
		DebugMessageN1("CAGuard::WaitFor: waiting %.0f", (Float64)inNanos);
	#endif

	struct timespec	theTimeSpec;
	static const UInt64	kNanosPerSecond = 1000000000ULL;
	if(inNanos > kNanosPerSecond)
	{
		theTimeSpec.tv_sec = inNanos / kNanosPerSecond;
		theTimeSpec.tv_nsec = inNanos % kNanosPerSecond;
	}
	else
	{
		theTimeSpec.tv_sec = 0;
		theTimeSpec.tv_nsec = inNanos;
	}
	
	#if	Log_TimedWaits || Log_Latency || Log_Average_Latency
		UInt64	theStartNanos = CAHostTimeBase::GetCurrentTimeInNanos();
	#endif

	mOwner = 0;

	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::WaitFor: thread %p is waiting on %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif

	OSStatus theError = pthread_cond_timedwait_relative_np(&mCondVar, &mMutex, &theTimeSpec);
	ThrowIf((theError != 0) && (theError != ETIMEDOUT), CAException(theError), "CAGuard::WaitFor: Wait got an error");
	mOwner = pthread_self();
	
	#if	Log_TimedWaits || Log_Latency || Log_Average_Latency
		UInt64	theEndNanos = CAHostTimeBase::GetCurrentTimeInNanos();
	#endif
	
	#if	Log_TimedWaits
		DebugMessageN1("CAGuard::WaitFor: waited  %.0f", (Float64)(theEndNanos - theStartNanos));
	#endif
	
	#if	Log_Latency
		DebugMessageN1("CAGuard::WaitFor: latency  %.0f", (Float64)((theEndNanos - theStartNanos) - inNanos));
	#endif
	
	#if	Log_Average_Latency
		++mAverageLatencyCount;
		mAverageLatencyAccumulator += (theEndNanos - theStartNanos) - inNanos;
		if(mAverageLatencyCount >= 50)
		{
			DebugMessageN2("CAGuard::WaitFor: average latency  %.3f ns over %ld waits", mAverageLatencyAccumulator / mAverageLatencyCount, mAverageLatencyCount);
			mAverageLatencyCount = 0;
			mAverageLatencyAccumulator = 0.0;
		}
	#endif

	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::WaitFor: thread %p waited on %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif

	return theError == ETIMEDOUT;
}

bool	CAGuard::WaitUntil(UInt64 inNanos)
{
	bool	theAnswer = false;
	UInt64	theCurrentNanos = CAHostTimeBase::GetCurrentTimeInNanos();
	
#if	Log_TimedWaits
	DebugMessageN2("CAGuard::WaitUntil: now: %.0f, requested: %.0f", (double)theCurrentNanos, (double)inNanos);
#endif
	
	if(inNanos > theCurrentNanos)
	{
		if((inNanos - theCurrentNanos) > 1000000000ULL)
		{
			DebugMessage("CAGuard::WaitUntil: about to wait for more than a second");
		}
		theAnswer = WaitFor(inNanos - theCurrentNanos);
	}
	else
	{
		DebugMessageN2("CAGuard::WaitUntil: Time has expired before waiting, now: %.0f, requested: %.0f", (double)theCurrentNanos, (double)inNanos);
	}

	return theAnswer;
}

void	CAGuard::Notify()
{
	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::Notify: thread %p is notifying %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif

	OSStatus theError = pthread_cond_signal(&mCondVar);
	ThrowIf(theError != 0, CAException(theError), "CAGuard::Notify: failed");
}

void	CAGuard::NotifyAll()
{
	#if	Log_WaitOwnership
		DebugPrintfRtn(DebugPrintfFile, "%p %.4f: CAGuard::NotifyAll: thread %p is notifying %s, owner: %p\n", pthread_self(), ((Float64)(CAHostTimeBase::GetCurrentTimeInNanos()) / 1000000.0), pthread_self(), mName, mOwner);
	#endif

	OSStatus theError = pthread_cond_broadcast(&mCondVar);
	ThrowIf(theError != 0, CAException(theError), "CAGuard::NotifyAll: failed");
}
