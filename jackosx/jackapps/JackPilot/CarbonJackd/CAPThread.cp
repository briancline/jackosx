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
	CAPThread.cp

=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

#include "CAPThread.h"
#include "CADebugMacros.h"
#include "CAException.h"

//	the extern "C" is to make cpp-precomp happy
extern "C"
{
#include <mach/mach.h>
}

//=============================================================================
//	CAPThread
//
//	This class wraps a pthread.
//=============================================================================

CAPThread::CAPThread(ThreadRoutine inThreadRoutine, void* inParameter, UInt32 inPriority)
:
	mPThread(0),
    mSpawningThreadPriority(getThreadPriority(pthread_self(), CAPTHREAD_SET_PRIORITY)),
	mThreadRoutine(inThreadRoutine),
	mThreadParameter(inParameter),
	mPriority(inPriority),
	mPeriod(0),
	mComputation(0),
	mConstraint(0),
	mIsPreemptible(true),
	mTimeConstraintSet(false)
{
}

CAPThread::CAPThread(ThreadRoutine inThreadRoutine, void* inParameter, UInt32 inPeriod, UInt32 inComputation, UInt32 inConstraint, bool inIsPreemptible)
:
	mPThread(0),
    mSpawningThreadPriority(getThreadPriority(pthread_self(), CAPTHREAD_SET_PRIORITY)),
	mThreadRoutine(inThreadRoutine),
	mThreadParameter(inParameter),
	mPriority(kDefaultThreadPriority),
	mPeriod(inPeriod),
	mComputation(inComputation),
	mConstraint(inConstraint),
	mIsPreemptible(inIsPreemptible),
	mTimeConstraintSet(true)
{
}

CAPThread::~CAPThread()
{
    //pthread_t jackThread = jackdmain_thread();
    //pthread_kill(jackThread,3);
    pthread_kill(mPThread,3);
}

UInt32	CAPThread::GetScheduledPriority()
{
    return CAPThread::getThreadPriority ( mPThread, CAPTHREAD_SCHEDULED_PRIORITY );
}

void	CAPThread::SetPriority(UInt32 inPriority)
{
	mPriority = inPriority;
	mTimeConstraintSet = false;
	if(mPThread != 0)
	{
        // We keep a reference to the spawning thread's priority around (initialized in the constructor), 
        // and set the importance of the child thread relative to the spawning thread's priority.
        thread_precedence_policy_data_t		thePrecedencePolicy;
        
        thePrecedencePolicy.importance = mPriority - mSpawningThreadPriority;
        thread_policy_set (pthread_mach_thread_np(mPThread), THREAD_PRECEDENCE_POLICY, (thread_policy_t)&thePrecedencePolicy, THREAD_PRECEDENCE_POLICY_COUNT);
    }
}

void	CAPThread::SetTimeConstraints(UInt32 inPeriod, UInt32 inComputation, UInt32 inConstraint, bool inIsPreemptible)
{
	mPeriod = inPeriod;
	mComputation = inComputation;
	mConstraint = inConstraint;
	mIsPreemptible = inIsPreemptible;
	mTimeConstraintSet = true;
	if(mPThread != 0)
	{
		thread_time_constraint_policy_data_t thePolicy;
		thePolicy.period = mPeriod;
		thePolicy.computation = mComputation;
		thePolicy.constraint = mConstraint;
		thePolicy.preemptible = mIsPreemptible;
		thread_policy_set(pthread_mach_thread_np(mPThread), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&thePolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
	}
}

void	CAPThread::Start()
{
	if(mPThread == 0)
	{
		OSStatus			theResult;
		pthread_attr_t		theThreadAttributes;
		
		theResult = pthread_attr_init(&theThreadAttributes);
		ThrowIf(theResult != 0, CAException(theResult), "CAPThread::Start: Thread attributes could not be created.");
		
		theResult = pthread_attr_setdetachstate(&theThreadAttributes, PTHREAD_CREATE_DETACHED);
		ThrowIf(theResult != 0, CAException(theResult), "CAPThread::Start: A thread could not be created in the detached state.");
		
		theResult = pthread_create(&mPThread, &theThreadAttributes, (ThreadRoutine)CAPThread::Entry, this);
		ThrowIf(theResult != 0, CAException(theResult), "CAPThread::Start: Could not create a thread.");
		
		pthread_attr_destroy(&theThreadAttributes);
		
		if(mTimeConstraintSet)
		{
			SetTimeConstraints(mPeriod, mComputation, mConstraint, mIsPreemptible);
		}
		else
		{
			SetPriority(mPriority);
		}
	}
}

void*	CAPThread::Entry(CAPThread* inCAPThread)
{
	void* theAnswer = NULL;
	if(inCAPThread->mThreadRoutine != NULL)
	{
		theAnswer = inCAPThread->mThreadRoutine(inCAPThread->mThreadParameter);
	}
	inCAPThread->mPThread = 0;
	return theAnswer;
}

//========================================================================
// Thread Priority Get
//========================================================================
UInt32 CAPThread::getThreadPriority (pthread_t inThread, int inPriorityKind)
{
    thread_basic_info_data_t			threadInfo;
	policy_info_data_t					thePolicyInfo;
	unsigned int						count;
    
    // get basic info
    count = THREAD_BASIC_INFO_COUNT;
    thread_info (pthread_mach_thread_np (inThread), THREAD_BASIC_INFO, (thread_info_t)&threadInfo, &count);
    
	switch (threadInfo.policy) {
		case POLICY_TIMESHARE:
			count = POLICY_TIMESHARE_INFO_COUNT;
			thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_TIMESHARE_INFO, (thread_info_t)&(thePolicyInfo.ts), &count);
            if (inPriorityKind == CAPTHREAD_SCHEDULED_PRIORITY) {
                return thePolicyInfo.ts.cur_priority;
            }
            return thePolicyInfo.ts.base_priority;
            break;
            
        case POLICY_FIFO:
			count = POLICY_FIFO_INFO_COUNT;
			thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_FIFO_INFO, (thread_info_t)&(thePolicyInfo.fifo), &count);
            if ( (thePolicyInfo.fifo.depressed) && (inPriorityKind == CAPTHREAD_SCHEDULED_PRIORITY) ) {
                return thePolicyInfo.fifo.depress_priority;
            }
            return thePolicyInfo.fifo.base_priority;
            break;
            
		case POLICY_RR:
			count = POLICY_RR_INFO_COUNT;
			thread_info(pthread_mach_thread_np (inThread), THREAD_SCHED_RR_INFO, (thread_info_t)&(thePolicyInfo.rr), &count);
			if ( (thePolicyInfo.rr.depressed) && (inPriorityKind == CAPTHREAD_SCHEDULED_PRIORITY) ) {
                return thePolicyInfo.rr.depress_priority;
            }
            return thePolicyInfo.rr.base_priority;
            break;
	}
    
    return 0;
}
