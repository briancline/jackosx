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
	CAPThread.h

=============================================================================*/
#if !defined(__CAPThread_h__)
#define __CAPThread_h__

//=============================================================================
//	Defines
//=============================================================================

// returns the thread's priority as it was last set by the API
#define CAPTHREAD_SET_PRIORITY				0
// returns the thread's priority as it was last scheduled by the Kernel
#define CAPTHREAD_SCHEDULED_PRIORITY		1

//=============================================================================
//	Includes
//=============================================================================

#include <CoreAudio/CoreAudioTypes.h>
#include <pthread.h>
#include <Jackd/jackd.h>

//=============================================================================
//	CAPThread
//
//	This class wraps a pthread.
//=============================================================================

class	CAPThread
{

//	Types
public:
	typedef void*	(*ThreadRoutine)(void* inParameter);

//	Constants
public:
	static const UInt32	kMinThreadPriority = 1;
	static const UInt32 kMaxThreadPriority = 63;
	static const UInt32 kDefaultThreadPriority = 31;

//	Construction/Destruction
public:
						CAPThread(ThreadRoutine inThreadRoutine, void* inParameter, UInt32 inPriority = kDefaultThreadPriority);
						CAPThread(ThreadRoutine inThreadRoutine, void* inParameter, UInt32 inPeriod, UInt32 inComputation, UInt32 inConstraint, bool inIsPreemptible);
	virtual				~CAPThread();

//	Properties
public:
	pthread_t			GetPThread() const { return mPThread; }
	bool				IsCurrentThread() const { return (0 != mPThread) && (pthread_self() == mPThread); }
	bool				IsRunning() const { return 0 != mPThread; }
	
	bool				IsTimeShareThread() const { return !mTimeConstraintSet; }
	UInt32				GetPriority() const { return mPriority; }
    UInt32				GetScheduledPriority();
    void				SetPriority(UInt32 inPriority);
    
	bool				IsTimeConstraintThread() const { return mTimeConstraintSet; }
	void				GetTimeConstraints(UInt32& outPeriod, UInt32& outComputation, UInt32& outConstraint, bool& outIsPreemptible) const { outPeriod = mPeriod; outComputation = mComputation; outConstraint = mConstraint; outIsPreemptible = mIsPreemptible; }
	void				SetTimeConstraints(UInt32 inPeriod, UInt32 inComputation, UInt32 inConstraint, bool inIsPreemptible);
	void				ClearTimeConstraints() { SetPriority(mPriority); }
		
//	Actions
public:
	virtual void		Start();

//	Implementation
protected:
	static void*		Entry(CAPThread* inCAPThread);
    static UInt32		getThreadPriority (pthread_t inThread, int inPriorityKind);
    
	pthread_t			mPThread;
    UInt32				mSpawningThreadPriority;
	ThreadRoutine		mThreadRoutine;
	void*				mThreadParameter;
	SInt32				mPriority;
	UInt32				mPeriod;
	UInt32				mComputation;
	UInt32				mConstraint;
	bool				mIsPreemptible;
	bool				mTimeConstraintSet;
    
};

#endif
