/*	Copyright � 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	JackRouterDevice.cpp
=============================================================================*/

//=============================================================================
//	Includes
//=============================================================================

//	Self Include
#include "JackRouterDevice.h"
#include "JARLog.h"
#include "bequite.h"

//	Internal Includes
#include "JackRouterControl.h"
#include "JackRouterPlugIn.h"
#include "JackRouterStream.h"

//	HPBase Includes
#include "HP_DeviceSettings.h"
#include "HP_HogMode.h"
#include "HP_IOCycleTelemetry.h"
#include "HP_IOProcList.h"
#include "HP_IOThread.h"

//	PublicUtility Includes
#include "CAAudioBufferList.h"
#include "CAAudioTimeStamp.h"
#include "CAAutoDisposer.h"
#include "CACFString.h"
#include "CADebugMacros.h"
#include "CAException.h"
#include "CAHostTimeBase.h"
#include "CALogMacros.h"
#include "CAMutex.h"

using namespace std;

//=============================================================================
//	JackRouterDevice
//=============================================================================

#define kAudioTimeFlags kAudioTimeStampSampleTimeValid|kAudioTimeStampHostTimeValid|kAudioTimeStampRateScalarValid

int JackRouterDevice::fInputChannels = 0;
int JackRouterDevice::fOutputChannels = 0;

bool JackRouterDevice::fAutoConnect = true;

bool JackRouterDevice::fDefaultInput = true;	
bool JackRouterDevice::fDefaultOutput = true;	
bool JackRouterDevice::fDefaultSystem = true;	

int JackRouterDevice::fBufferSize;
float JackRouterDevice::fSampleRate;

char JackRouterDevice::fCoreAudioDriverUID[128];

// Additional thread for deferred commands execution
CommandThread::CommandThread(JackRouterDevice* inDevice):
	mDevice(inDevice),
	mStopWorkLoop(false),
	mCommandGuard("CommandGuard"),
	mCommandThread(reinterpret_cast<CAPThread::ThreadRoutine>(ThreadEntry), this, CAPThread::kDefaultThreadPriority)
{}

CommandThread::~CommandThread()
{}

void*	CommandThread::ThreadEntry(CommandThread* inIOThread)
{
	inIOThread->WorkLoop();
	return NULL;
}

void	CommandThread::WorkLoop()
{
	while (!mStopWorkLoop) {
		if (mDevice->GetCommands() > 0) {
			mDevice->GetIOGuard()->Lock();
			mDevice->ExecuteAllCommands(); 
			mDevice->GetIOGuard()->Unlock(); 
		}
		usleep(200000);
	}
	
	mCommandGuard.NotifyAll();
}

void	CommandThread::Start()
{
	mCommandThread.Start();
}

void	CommandThread::Stop()
{
	mStopWorkLoop = true;
	mCommandGuard.Wait();
}

JackRouterDevice::JackRouterDevice(AudioDeviceID inAudioDeviceID, JackRouterPlugIn* inPlugIn)
:HP_Device(inAudioDeviceID, kAudioDeviceClassID, inPlugIn, 1, false),
	mSHPPlugIn(inPlugIn),
	mIOGuard("IOGuard"),
	fClient(NULL),
	fInputList(NULL),
	fOutputList(NULL),
	fOuputListTemp(NULL),
	fFirstActivate(true)
{}

JackRouterDevice::~JackRouterDevice()
{}

void	JackRouterDevice::Initialize()
{
	JARLog("JackRouterDevice Initialize\n");
	
	HP_Device::Initialize();
	
	//	allocate the IO thread implementation
	mCommandThread = new CommandThread(this);
	mCommandThread->Start();
	
	// JACK
	fInputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * JackRouterDevice::fInputChannels);
    assert(fInputList);
    fOutputList = (AudioBufferList*)malloc(sizeof(UInt32) + sizeof(AudioBuffer) * JackRouterDevice::fOutputChannels);
    assert(fOutputList);

    fInputList->mNumberBuffers = JackRouterDevice::fInputChannels;
    fOutputList->mNumberBuffers = JackRouterDevice::fOutputChannels;

    fOuputListTemp = (float**)malloc(sizeof(float*) * JackRouterDevice::fOutputChannels);
    assert(fOuputListTemp);

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        fOuputListTemp[i] = (float*)malloc(sizeof(float) * JackRouterDevice::fBufferSize);
		assert(fOuputListTemp[i]);
        memset(fOuputListTemp[i], 0, JackRouterDevice::fBufferSize * sizeof(float));
     }

    for (int i = 0; i < MAX_JACK_PORTS; i++) {
        fInputPortList[i] = NULL;
        fOutputPortList[i] = NULL;
    }

 	//	create the streams
	CreateStreams();
	
	//  set the default buffer size before we go any further
	mIOBufferFrameSize = fBufferSize;
	JARLog("JackRouterDevice Initialize OK\n");
}

void	JackRouterDevice::Teardown()
{
	JARLog("JackRouterDevice Teardown\n");
		
	//	stop things
	Do_StopAllIOProcs();
	ReleaseStreams();	
	
	// JACK
    free(fInputList);
    free(fOutputList);

    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++)
        free(fOuputListTemp[i]);
    free(fOuputListTemp);
	
	mCommandThread->Stop();
	delete mCommandThread;
	
	HP_Device::Teardown();
}

void	JackRouterDevice::Finalize()
{
	//	Finalize() is called in place of Teardown() when we're being lazy about
	//	cleaning up. The idea is to do as little work as possible here.
	
	//	go through the streams and finalize them
	JackRouterStream* theStream;
	UInt32 theStreamIndex;
	UInt32 theNumberStreams;
	
	//	input
	theNumberStreams = GetNumberStreams(true);
	for(theStreamIndex = 0; theStreamIndex != theNumberStreams; ++theStreamIndex)
	{
		theStream = static_cast<JackRouterStream*>(GetStreamByIndex(true, theStreamIndex));
		theStream->Finalize();
	}
	
	//	output
	theNumberStreams = GetNumberStreams(false);
	for(theStreamIndex = 0; theStreamIndex != theNumberStreams; ++theStreamIndex)
	{
		theStream = static_cast<JackRouterStream*>(GetStreamByIndex(false, theStreamIndex));
		theStream->Finalize();
	}
}

CFStringRef	JackRouterDevice::CopyDeviceName() const
{
	CFStringRef theAnswer = CFSTR("JackRouter");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackRouterDevice::CopyDeviceManufacturerName() const
{
	CFStringRef theAnswer = CFSTR("Grame");
	CFRetain(theAnswer);
	return theAnswer;
}

CFStringRef	JackRouterDevice::CopyDeviceUID() const
{
	CFStringRef theAnswer = CFSTR("JackRouter:0");
	CFRetain(theAnswer);
	return theAnswer;
}

bool JackRouterDevice::CanBeDefaultDevice(bool /*inIsInput */, bool inIsSystem) const
{
	return (inIsSystem) ? false : true;
}

bool	JackRouterDevice::HasProperty(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	JARLog("JackRouterDevice::HasProperty \n");
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
		case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
			JARLog("JackRouterDevice::HasProperty JACK special\n");
			theAnswer = true;
			break;
		
		default:
			theAnswer = HP_Device::HasProperty(inAddress);
			break;
	};
	
	return theAnswer;
}

bool	JackRouterDevice::IsPropertySettable(const AudioObjectPropertyAddress& inAddress) const
{
	bool theAnswer = false;
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
		case kAudioDevicePropertyBufferFrameSize:
		case kAudioDevicePropertyBufferSize:
			theAnswer = false;
			break;
			
		case kAudioDevicePropertyIOProcStreamUsage:
			theAnswer = true;
			break;

		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
            theAnswer = false;
            break;

        case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
            theAnswer = true;
            break;
			
		case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
            theAnswer = false;
            break;

		case kAudioDevicePropertyIOCycleUsage:
			theAnswer = false;
			break;
		
		default:
			theAnswer = HP_Device::IsPropertySettable(inAddress);
			break;
	};
	
	return theAnswer;
}

UInt32	JackRouterDevice::GetPropertyDataSize(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData) const
{
	UInt32	theAnswer = 0;
	
	JARLog("JackRouterDevice::GetPropertyDataSize\n");
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	switch(inAddress.mSelector)
	{
			
		case kAudioDevicePropertyGetJackClient:
        case kAudioDevicePropertyReleaseJackClient:
			JARLog("JackRouterDevice::GetPropertyDataSize kAudioDevicePropertyGetJackClient\n");
            theAnswer = sizeof(jack_client_t*);
            break;

        case kAudioDevicePropertyAllocateJackPortVST:
        case kAudioDevicePropertyAllocateJackPortAU:
        case kAudioDevicePropertyReleaseJackPortVST:
        case kAudioDevicePropertyReleaseJackPortAU:
        case kAudioDevicePropertyDeactivateJack:
        case kAudioDevicePropertyActivateJack:
            theAnswer = sizeof(UInt32);
            break;
			
		case kAudioDevicePropertyGetJackPortVST:
        case kAudioDevicePropertyGetJackPortAU:
            theAnswer = sizeof(float*);
            break;
		
		default:
			theAnswer = HP_Device::GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData);
			break;
	};
	
	return theAnswer;
}

void	JackRouterDevice::GetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32& ioDataSize, void* outData) const
{
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(const_cast<JackRouterDevice*>(this)->GetStateMutex());
	
	JARLog("JackRouterDevice::GetPropertyData\n");
	
	switch(inAddress.mSelector)
	{
		
		case kAudioDevicePropertyGetJackClient: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackClient\n");
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceUID");
			if (fClient)
				*static_cast<jack_client_t**>(outData) = fClient;
			else
				 throw CAException(kAudioHardwareIllegalOperationError);
			break;
			
		case kAudioDevicePropertyReleaseJackClient:
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyReleaseJackClient\n");
			ThrowIf(ioDataSize != GetPropertyDataSize(inAddress, inQualifierDataSize, inQualifierData), CAException(kAudioHardwareBadPropertySizeError), "JackRouterDevice::GetPropertyData: wrong data size for kAudioDevicePropertyDeviceUID");
			break;
			
		case kAudioDevicePropertyGetJackPortVST: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackPortVST\n");
			*static_cast<float**>(outData) = const_cast<JackRouterDevice*>(this)->GetPlugInPortVST(ioDataSize);
			break;
		
        case kAudioDevicePropertyGetJackPortAU: 
			JARLog("JackRouterDevice::GetPropertyData kAudioDevicePropertyGetJackPortAU\n");
			*static_cast<float**>(outData) = const_cast<JackRouterDevice*>(this)->GetPlugInPortAU(ioDataSize);
			break;
		
		default:
			HP_Device::GetPropertyData(inAddress, inQualifierDataSize, inQualifierData, ioDataSize, outData);
			break;
	};
}

void	JackRouterDevice::SetPropertyData(const AudioObjectPropertyAddress& inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData, const AudioTimeStamp* inWhen)
{
	JARLog("JackRouterDevice::SetPropertyData\n");
	
	//	take and hold the state mutex
	CAMutex::Locker theStateMutex(GetStateMutex());
	
	switch(inAddress.mSelector)
	{
		
		 case kAudioDevicePropertyAllocateJackPortVST: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyAllocateJackPortVST\n");
			const_cast<JackRouterDevice*>(this)->AllocatePlugInPortVST(inDataSize);
			break;
     
            // Special Property to allocate Jack port from AU plug-in code
        case kAudioDevicePropertyAllocateJackPortAU: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyAllocateJackPortAU\n");
            const_cast<JackRouterDevice*>(this)->AllocatePlugInPortAU(inDataSize);
			break;
     
            // Special Property to release Jack port from VST plug-in code
        case kAudioDevicePropertyReleaseJackPortVST: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyReleaseJackPortVST\n");
            const_cast<JackRouterDevice*>(this)->ReleasePlugInPortVST(inDataSize);
			break;
     
            // Special Property to release Jack port from AU plug-in code
        case kAudioDevicePropertyReleaseJackPortAU: 
			JARLog("JackRouterDevice::SetPropertyData kAudioDevicePropertyReleaseJackPortAU\n");
			const_cast<JackRouterDevice*>(this)->ReleasePlugInPortAU(inDataSize);
			break;
      
            // Special Property to deactivate jack from plug-in code
        case kAudioDevicePropertyDeactivateJack: 
			break;
    
        case kAudioDevicePropertyActivateJack: 
			break;
     		
		case kAudioDevicePropertyIOProcStreamUsage: {
			AudioHardwareIOProcStreamUsage* inData1 = (AudioHardwareIOProcStreamUsage*)inData;
			JARLog("DeviceSetProperty inAddress.mScope %ld : mNumberStreams %d\n", inAddress.mScope, inData1->mNumberStreams);
			
			if (inAddress.mScope == kAudioDevicePropertyScopeInput && fClient) {
				JARLog("DeviceSetProperty input : mNumberStreams %d\n", inData1->mNumberStreams);
				for (UInt32 i = 0; i < inData1->mNumberStreams; i++) {
					if (inData1->mStreamIsOn[i]) {
						if (fInputPortList[i] == 0) {
							char in_port_name [JACK_PORT_NAME_LEN];
							sprintf(in_port_name, "in%lu", i + 1);
							fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
							JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
						}
					} else {
						if (fInputPortList[i]) {
							jack_port_unregister(fClient, fInputPortList[i]);
							fInputPortList[i] = 0;
							JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
						}
					}
					JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage input inData->mStreamIsOn %ld \n", inData1->mStreamIsOn[i]);
				}
			}
			
			if (inAddress.mScope == kAudioDevicePropertyScopeOutput && fClient) {
				JARLog("DeviceSetProperty output : mNumberStreams %d\n", inData1->mNumberStreams);
				for (UInt32 i = 0; i < inData1->mNumberStreams; i++) {
					if (inData1->mStreamIsOn[i]) {
						if (fOutputPortList[i] == 0) {
							char out_port_name [JACK_PORT_NAME_LEN];
							sprintf(out_port_name, "out%lu", i + 1);
							fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
							JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_register %ld \n", i);
						}
					} else {
						if (fOutputPortList[i]) {
							jack_port_unregister(fClient, fOutputPortList[i]);
							fOutputPortList[i] = 0;
							JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage jack_port_unregister %ld \n", i);
						}
					}
					JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage input inData->mStreamIsOn %ld \n", inData1->mStreamIsOn[i]);
				}
			}
			
			// Autoconnect is only done for the first activation
			if (fFirstActivate) {
				AutoConnect();
				fFirstActivate = false;
			} else {
				RestoreConnections();
			}
			
			HP_Device::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
		}
				
		default:
			HP_Device::SetPropertyData(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, inWhen);
			break;
	};
}

void	JackRouterDevice::AddIOProc(AudioDeviceIOProc inProc, void* inClientData)
{
	HP_Device::AddIOProc(inProc, inClientData);
	JARLog("JackRouterDevice::AddIOProc\n");
	
	// First IO proc start JACK
	if (mIOProcList->GetNumberIOProcs() == 1) {
		Open();
		AllocatePorts();
		Activate();
	}
}

void	JackRouterDevice::RemoveIOProc(AudioDeviceIOProc inProc)
{
	HP_Device::RemoveIOProc(inProc);
	JARLog("JackRouterDevice::RemoveIOProc\n");
	
	// Last IO proc stop JACK
	if (mIOProcList->GetNumberIOProcs() == 0) {
		Desactivate();
		DisposePorts();
		Close();
	}
}

AudioDeviceIOProcID	JackRouterDevice::Do_CreateIOProcID(AudioDeviceIOProc inProc, void* inClientData)
{
	AudioDeviceIOProcID res = HP_Device::Do_CreateIOProcID(inProc, inClientData);
	JARLog("JackRouterDevice::Do_CreateIOProcID\n");
	
	// First IO proc start JACK
	if (mIOProcList->GetNumberIOProcs() == 1) {
		Open();
		AllocatePorts();
		Activate();
	}
	
	return res;
}	

void	JackRouterDevice::StopAllIOProcs()
{
	HP_Device::StopAllIOProcs();
	JARLog("JackRouterDevice::StopAllIOProcs\n");
	
	// Last IO proc stop JACK
	if (mIOProcList->GetNumberIOProcs() == 0) {
		Desactivate();
		DisposePorts();
		Close();
	}
}

void	JackRouterDevice::PropertyListenerAdded(const AudioObjectPropertyAddress& inAddress)
{
	JARLog("JackRouterDevice::PropertyListenerAdded\n");
	HP_Object::PropertyListenerAdded(inAddress);
}

void	JackRouterDevice::Do_StartIOProc(AudioDeviceIOProc inProc)
{
	//	start
	HP_Device::Do_StartIOProc(inProc);
}

void	JackRouterDevice::Do_StartIOProcAtTime(AudioDeviceIOProc inProc, AudioTimeStamp& ioStartTime, UInt32 inStartTimeFlags)
{
    //	start
	HP_Device::Do_StartIOProcAtTime(inProc, ioStartTime, inStartTimeFlags);
}

void	JackRouterDevice::Do_StopIOProc(AudioDeviceIOProc inProc)
{
	//	stop
	HP_Device::Do_StopIOProc(inProc);
}

void	JackRouterDevice::StartIOEngine()
{
	JARLog("JackRouterDevice::StartIOEngine\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	}
}

void	JackRouterDevice::StartIOEngineAtTime(const AudioTimeStamp&  /*inStartTime*/, UInt32 /*inStartTimeFlags*/)
{
	JARLog("JackRouterDevice::StartIOEngineAtTime\n");
	if (!IsIOEngineRunning()) {
		StartHardware();
	}
}

void	JackRouterDevice::StopIOEngine()
{
	JARLog("JackRouterDevice::StopIOEngine\n");
	if (IsIOEngineRunning()) {
		StopHardware();
	}
}

void	JackRouterDevice::StartHardware()
{
	//	the calling thread must have already locked the Guard prior to calling this method
	JARLog("JackRouterDevice::StartHardware\n");

	//	set the device state to know the engine is running
	IOEngineStarted();
	
	//	notify clients that the engine is running
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);	
}

void	JackRouterDevice::StopHardware()
{
	JARLog("JackRouterDevice::StopHardware\n");
	
	//	set the device state to know the engine has stopped
	IOEngineStopped();
	
	//	Notify clients that the IO callback is stopping
	CAPropertyAddress theIsRunningAddress(kAudioDevicePropertyDeviceIsRunning);
	PropertiesChanged(1, &theIsRunningAddress);
}

bool	JackRouterDevice::IsSafeToExecuteCommand()
{
	bool theAnswer = true;

	if (fClient) {
		JARLog("JackRouterDevice::IsSafeToExecuteCommand jack_client_thread_id = %ld pthread_self = %ld\n", jack_client_thread_id(fClient), pthread_self());
		theAnswer = jack_client_thread_id(fClient) != pthread_self();
	}
	
	JARLog("JackRouterDevice::IsSafeToExecuteCommand theAnswer = %ld\n", theAnswer);
	return theAnswer;
}

bool	JackRouterDevice::StartCommandExecution(void** outSavedCommandState)
{
	JARLog("JackRouterDevice::StartCommandExecution \n");
	*outSavedCommandState = mIOGuard.Lock() ? (void*)1 : (void*)0;
	return true;
}

void	JackRouterDevice::FinishCommandExecution(void* inSavedCommandState)
{
	JARLog("JackRouterDevice::FinishCommandExecution \n");
	if (inSavedCommandState != 0) {
		mIOGuard.Unlock();
	}
}

void	JackRouterDevice::GetCurrentTime(AudioTimeStamp& outTime)
{
	ThrowIf(!IsIOEngineRunning(), CAException(kAudioHardwareNotRunningError), "JackRouterDevice::GetCurrentTime: can't because the engine isn't running");
	
	outTime.mSampleTime = jack_frame_time(fClient);
	outTime.mHostTime = AudioGetCurrentHostTime();
	outTime.mRateScalar = 1.0;
	outTime.mWordClockTime = 0;
	outTime.mFlags = kAudioTimeFlags;
}

void	JackRouterDevice::SafeGetCurrentTime(AudioTimeStamp& outTime)
{
	//	The difference between GetCurrentTime and SafeGetCurrentTime is that GetCurrentTime should only
	//	be called in situations where the device state or clock state is in a known good state, such
	//	as during the IO cycle. Being in a known good state allows GetCurrentTime to bypass any
	//	locks that ensure coherent cross-thread access to the device time base info.
	//	SafeGetCurrentTime, then, will be called when the state is in question and all the locks should
	//	be obeyed.
	
	//	Our state here in the sample device has no such threading issues, so we pass this call on
	//	to GetCurrentTime.
	GetCurrentTime(outTime);
}

void	JackRouterDevice::TranslateTime(const AudioTimeStamp& inTime, AudioTimeStamp& outTime)
{
	//	the input time stamp has to have at least one of the sample or host time valid
	ThrowIf((inTime.mFlags & kAudioTimeStampSampleHostTimeValid) == 0, CAException(kAudioHardwareIllegalOperationError), "JackRouterDevice::TranslateTime: have to have either sample time or host time valid on the input");
	ThrowIf(!IsIOEngineRunning(), CAException(kAudioHardwareNotRunningError), "JackRouterDevice::TranslateTime: can't because the engine isn't running");

	// Simply copy inTime ==> outTime for now
	memcpy(&outTime, &inTime, sizeof(AudioTimeStamp));
}

UInt32	JackRouterDevice::GetMinimumIOBufferFrameSize() const
{
	return fBufferSize;
}

UInt32	JackRouterDevice::GetMaximumIOBufferFrameSize() const
{
	return fBufferSize;
}

void	JackRouterDevice::GetNearestStartTime(AudioTimeStamp& /*ioRequestedStartTime*/, UInt32 /*inFlags*/)
{
	JARLog("JackRouterDevice::GetNearestStartTime\n");
}

void	JackRouterDevice::CreateStreams()
{
	//  common variables
	OSStatus		theError = 0;
	AudioObjectID   theNewStreamID = 0;
	JackRouterStream*		theStream = NULL;

	//  create a vector of AudioStreamIDs to hold the stream ids we are creating
	std::vector<AudioStreamID> theStreamIDs;
	
	for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
	
		//  instantiate an AudioStream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if(theError == 0)
		{
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, true, 1, fSampleRate);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}
	
	for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {

		//  claim a stream ID for the stream
	#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareClaimAudioStreamID(mSHPPlugIn->GetInterface(), GetObjectID(), &theNewStreamID);
	#else
		theError = AudioObjectCreate(mSHPPlugIn->GetInterface(), GetObjectID(), kAudioStreamClassID, &theNewStreamID);
	#endif
		if(theError == 0)
		{
			//  create the stream
			theStream = new JackRouterStream(theNewStreamID, mSHPPlugIn, this, false, 1, fSampleRate);
			theStream->Initialize();
			
			//	add to the list of streams in this device
			AddStream(theStream);
			
			//  store the new stream ID
			theStreamIDs.push_back(theNewStreamID);
		}
	}

	//  now tell the HAL about the new stream IDs
	if(theStreamIDs.size() != 0)
	{
		//	set the object state mutexes
		for(std::vector<AudioStreamID>::iterator theIterator = theStreamIDs.begin(); theIterator != theStreamIDs.end(); std::advance(theIterator, 1))
		{
			HP_Object* theObject = HP_Object::GetObjectByID(*theIterator);
			if(theObject != NULL)
			{
				HP_Object::SetObjectStateMutexForID(*theIterator, theObject->GetObjectStateMutex());
			}
		}
		
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		theError = AudioHardwareStreamsCreated(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()));
#else
		theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), GetObjectID(), theStreamIDs.size(), &(theStreamIDs.front()), 0, NULL);
#endif
		ThrowIfError(theError, CAException(theError), "JackRouterDevice::CreateStreams: couldn't tell the HAL about the streams");
	}
}

void	JackRouterDevice::CreateForHAL(AudioDeviceID theNewDeviceID)
{
	JARLog("CreateForHAL\n");
	SetObjectID(theNewDeviceID);  // setup the new deviceID
	CreateStreams();
}

void	JackRouterDevice::ReleaseStreams()
{
	//	This method is only called when tearing down, so there isn't any need to inform the HAL about changes
	//	since the HAL has long since released it's internal representation of these stream objects. Note that
	//	if this method needs to be called outside of teardown, it would need to be modified to call
	//	AudioObjectsPublishedAndDied (or AudioHardwareStreamsDied on pre-Tiger systems) to notify the HAL about
	//	the state change.
	while(GetNumberStreams(true) > 0)
	{
		//	get the stream
		JackRouterStream* theStream = static_cast<JackRouterStream*>(GetStreamByIndex(true, 0));
		
		//	remove the object state mutex
		HP_Object::SetObjectStateMutexForID(theStream->GetObjectID(), NULL);

		//	remove it from the lists
		RemoveStream(theStream);
		
		//	toss it
		theStream->Teardown();
		delete theStream;
	}
	
	while(GetNumberStreams(false) > 0)
	{
		//	get the stream
		JackRouterStream* theStream = static_cast<JackRouterStream*>(GetStreamByIndex(false, 0));
		
		//	remove the object state mutex
		HP_Object::SetObjectStateMutexForID(theStream->GetObjectID(), NULL);

		//	remove it from the lists
		RemoveStream(theStream);
		
		//	toss it
		theStream->Teardown();
		delete theStream;
	}
}

void	JackRouterDevice::ReleaseFromHAL()
{
		AudioObjectID theObjectID = GetObjectID();
#if	(MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)
		OSStatus theError = AudioHardwareDevicesDied(mSHPPlugIn->GetInterface(), 1, &theObjectID);
#else
		OSStatus theError = AudioObjectsPublishedAndDied(mSHPPlugIn->GetInterface(), kAudioObjectSystemObject, 0, NULL, 1, &theObjectID);
#endif
		AssertNoError(theError, "JackRouterPlugIn::Teardown: got an error telling the HAL a device died");
		
		Destroy();
}

// JACK

void JackRouterDevice::SaveConnections()
{
	if (!fClient)
        return;

    JARLog("--------------------------------------------------------\n");
    JARLog("SaveConnections\n");

	const char** connections;
    fConnections.clear();

    for (int i = 0; i < fInputChannels; ++i) {
        if (fInputPortList[i] && (connections = jack_port_get_connections(fInputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(connections[j], jack_port_name(fInputPortList[i])));
            }
            free(connections);
        }
    }

    for (int i = 0; i < fOutputChannels; ++i) {
        if (fOutputPortList[i] && (connections = jack_port_get_connections(fOutputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(jack_port_name(fOutputPortList[i]), connections[j]));
            }
            free(connections);
        }
    }

    if (JAR_fDebug) {
	
        list<pair<string, string> >::const_iterator it;
		for (it = fConnections.begin(); it != fConnections.end(); it++) {
            pair<string, string> connection = *it;
            JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        }
    }
}

void JackRouterDevice::RestoreConnections()
{
    JARLog("--------------------------------------------------------\n");
    JARLog("RestoreConnections size = %ld\n", fConnections.size());

    list<pair<string, string> >::const_iterator it;

    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string, string> connection = *it;
        JARLog("connections : %s %s\n", connection.first.c_str(), connection.second.c_str());
        jack_connect(fClient, connection.first.c_str(), connection.second.c_str());
    }
}

bool JackRouterDevice::AutoConnect()
{
    const char** ports;

    if (fAutoConnect) {

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) == NULL) {
            JARLog("cannot find any physical capture ports\n");
        } else {

            for (int i = 0; i < fInputChannels; i++) {
                if (JAR_fDebug) {
                    if (ports[i])
                        JARLog("ports[i] %s\n", ports[i]);
                    if (fInputPortList[i] && jack_port_name(fInputPortList[i]))
                        JARLog("jack_port_name(fInputPortList[i]) = %s\n", jack_port_name(fInputPortList[i]));
                }

                // Stop condition
                if (ports[i] == 0)
                    break;

                if (fInputPortList[i] && jack_port_name(fInputPortList[i])) {
                    if (jack_connect(fClient, ports[i], jack_port_name(fInputPortList[i]))) {
                        JARLog("cannot connect input ports\n");
                    }
                } 
            }
            free(ports);
        }

        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) == NULL) {
            JARLog("cannot find any physical playback ports\n");
        } else {

            for (int i = 0; i < fOutputChannels; i++) {
                if (JAR_fDebug) {
                    if (ports[i])
                        JARLog("ports[i] %s\n", ports[i]);
                    if (fOutputPortList[i] && jack_port_name(fOutputPortList[i]))
                        JARLog("jack_port_name(fOutputPortList[i]) %s\n", jack_port_name(fOutputPortList[i]));
                }

                // Stop condition
                if (ports[i] == 0)
                    break;

                if (fOutputPortList[i] && jack_port_name(fOutputPortList[i])) {
                    if (jack_connect(fClient, jack_port_name(fOutputPortList[i]), ports[i])) {
                        JARLog("cannot connect ouput ports\n");
                    }
                }
            }
            free(ports);
        }
    }

    return true;
}

static void SetTime(AudioTimeStamp* timeVal, long curTime, UInt64 time)
{
    timeVal->mSampleTime = curTime;
    timeVal->mHostTime = time;
    timeVal->mRateScalar = 1.0;
    timeVal->mWordClockTime = 0;
    timeVal->mFlags = kAudioTimeFlags;
}

bool JackRouterDevice::AllocatePorts()
{
    char in_port_name[JACK_PORT_NAME_LEN];

    JARLog("AllocatePorts fInputChannels = %ld fOutputChannels = %ld \n", fInputChannels, fOutputChannels);

    for (long i = 0; i < JackRouterDevice::fInputChannels; i++) {
        sprintf(in_port_name, "in%ld", i + 1);
        if ((fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL)
            goto error;
        fInputList->mBuffers[i].mNumberChannels = 1;
        fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
    }

    char out_port_name[JACK_PORT_NAME_LEN];

    for (long i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        sprintf(out_port_name, "out%ld", i + 1);
        if ((fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL)
            goto error;
        fOutputList->mBuffers[i].mNumberChannels = 1;
        fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
    }

    return true;

error:

    JARLog("Cannot register ports\n");
    DisposePorts();
    return false;
}

void JackRouterDevice::DisposePorts()
{
    JARLog("DisposePorts\n");

    for (long i = 0; i < JackRouterDevice::fInputChannels; i++) {
        if (fInputPortList[i]) {
			JARLog("DisposePorts input %ld\n",i);
            jack_port_unregister(fClient, fInputPortList[i]);
            fInputPortList[i] = 0;
        }
    }

    for (long i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        if (fOutputPortList[i]) {
			JARLog("DisposePorts output %ld\n",i);
            jack_port_unregister(fClient, fOutputPortList[i]);
            fOutputPortList[i] = 0;
        }
    }
}

int JackRouterDevice::Process(jack_nframes_t nframes, void* arg)
{
    AudioTimeStamp inNow;
    AudioTimeStamp inInputTime;
    AudioTimeStamp inOutputTime;
	bool wasLocked;
	OSStatus err;
	JackRouterDevice* client = (JackRouterDevice*)arg;
	
	// Lock is on, so drop this cycle.
	if (!client->mIOGuard.Try(wasLocked)) {
		JARLog("JackRouterDevice::Process LOCK ON\n"); 
		return 0;
	}
	
	//JARLog("Process \n");
	
    UInt64 time = AudioGetCurrentHostTime();
    UInt64 curTime = jack_frame_time(client->fClient);
	SetTime(&inNow, curTime, time);
	SetTime(&inInputTime, curTime - JackRouterDevice::fBufferSize, time);
    SetTime(&inOutputTime, curTime + JackRouterDevice::fBufferSize, time);
	
	// One IOProc
  	if (client->mIOProcList->GetNumberIOProcs() == 1) {
		//JARLog("GetNumberIOProcs == 1 \n");

    	HP_IOProc* proc = client->mIOProcList->GetIOProcByIndex(0);

    	if (proc->IsEnabled()) {
		
			if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {  // A VERIFIER
			
				//JARLog("Process GetStreamUsage\n");
			
				// Only set up buffers that are really needed
                for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
                    if (proc->IsStreamEnabled(true, i) && client->fInputPortList[i]) {
                        client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                    } else {
                        client->fInputList->mBuffers[i].mData = NULL;
                    }
                }

                for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                    if (proc->IsStreamEnabled(false, i)) {
						memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
						client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                    } else {
                        client->fOutputList->mBuffers[i].mData = NULL;
                    }
                }

            } else {
			
				//JARLog("Process NO GetStreamUsage\n");
			
				// Non Interleaved
                for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
					if (client->fInputPortList[i]) {
						client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
					} else {
						//JARLog("JackRouterDevice::Process client->fInputPortList[i] %d is null\n",i);
					}
                }

                for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
					memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
                    client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                }
            }
			
			// Unlock before calling IO proc
			client->mIOGuard.Unlock();
			
            err = (proc->GetIOProc()) (client->mObjectID,
										&inNow,
										client->fInputList,
										&inInputTime,
										client->fOutputList,
										&inOutputTime,
										proc->GetClientData());

            if (JAR_fDebug && err != kAudioHardwareNoError) {
				JARLog("Process error = %ld\n", err);
            }

            // Copy intermediate buffer in client buffer
            for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
				if (client->fOutputPortList[i]) {
					float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
					memcpy(output, client->fOuputListTemp[i], nframes * sizeof(float));
				}
            }
			
        } else {
		
			for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
				if (client->fOutputPortList[i]) {
					float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
					memset(output, 0, nframes * sizeof(float));
				}
			}
			
			// Final Unlock
			client->mIOGuard.Unlock();
		}

    } else if (client->mIOProcList->GetNumberIOProcs() > 1) { // Several IOProc : need mixing

		//JARLog("GetNumberIOProcs > 1 size = %d\n", client->mIOProcList->GetNumberIOProcs());
			
		bool firstproc[JackRouterDevice::fOutputChannels];
		for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) 
			firstproc[i] = true;
			
		for (UInt32 k = 0; k < client->mIOProcList->GetNumberIOProcs(); k++) {

        	HP_IOProc* proc = client->mIOProcList->GetIOProcByIndex(k);
		
			if (proc && proc->IsEnabled()) { // If proc is started

                if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {
				
					//JARLog("Process GetStreamUsage input YES k = %d proc = %ld\n", k, proc->GetIOProc());

                    // Only set up buffers that are really needed
                    for (int i = 0; i < JackRouterDevice::fInputChannels; i++) {
                        if (proc->IsStreamEnabled(true, i)) {
                            client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
                        } else {
                            client->fInputList->mBuffers[i].mData = NULL;
                        }
                    }

                    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
                        // Use an intermediate mixing buffer
                        if (proc->IsStreamEnabled(false, i)) {
							memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
							client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
                        } else {
                            client->fOutputList->mBuffers[i].mData = NULL;
                        }
                    }

                } else {
				
					//JARLog("Process GetStreamUsage input NO k = %d proc = %ld\n", k, proc->GetIOProc());

                    for (int i = 0; i < fInputChannels; i++) {
						if (client->fInputPortList[i]) {
							client->fInputList->mBuffers[i].mData = (float*)jack_port_get_buffer(client->fInputPortList[i], nframes);
						}else {
							//JARLog("JackRouterDevice::Process client->fInputPortList[i] %d is null\n",i);
						}
                    }

                    for (int i = 0; i < fOutputChannels; i++) {
                        // Use an intermediate mixing buffer
						memset(client->fOuputListTemp[i], 0, nframes * sizeof(float));
                        client->fOutputList->mBuffers[i].mData = client->fOuputListTemp[i];
					}
                }

				// Unlock before calling IO proc
				client->mIOGuard.Unlock();

                err = (proc->GetIOProc()) (client->mObjectID,
											&inNow,
											client->fInputList,
											&inInputTime,
											client->fOutputList,
											&inOutputTime,
											proc->GetClientData());

				if (JAR_fDebug && err != kAudioHardwareNoError) {
					JARLog("Process error = %ld\n", err);
				}
				
				// Lock is on, so drop this cycle.
				if (!client->mIOGuard.Try(wasLocked)) {
					JARLog("JackRouterDevice::Process LOCK ON\n"); 
					return 0;
				}
				
                // Only mix buffers that are really needed
                if (proc->GetStreamUsage(true).size() > 0 || proc->GetStreamUsage(false).size() > 0) {
				
					//JARLog("Process GetStreamUsage output YES k = %d proc = %ld\n", k, proc->GetIOProc());
								
                    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
					
						//JARLog("Process GetStreamUsage YES i = %ld\n", i);
                        
                        if (proc->IsStreamEnabled(false, i)) {
							float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
							if (firstproc[i]) {	// first proc : copy
								//JARLog("Process GetStreamUsage YES first proc : copy = %ld\n", proc->GetIOProc());
                                memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
								firstproc[i] = false;
                            } else {			// other proc : mix
								//JARLog("Process GetStreamUsage YES other proc : mix = %ld\n", proc->GetIOProc());
                                for (UInt32 j = 0; j < nframes; j++) {
                                    output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
                                }
                            }
                        } else {
                			if (client->fOutputPortList[i] && firstproc[i]) {
								float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
								memset(output, 0, nframes * sizeof(float));
								firstproc[i] = false;
							}
                        }
                    }

                } else {
				
					//JARLog("Process GetStreamUsage output NO k = %d proc = %ld\n", k, proc->GetIOProc());
					
                    for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
						if (client->fOutputPortList[i]) {
							float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
							if (firstproc[i]) {	// first proc : copy
								//JARLog("Process GetStreamUsage NO first proc : copy = %ld\n", proc->GetIOProc());
								memcpy(output, (float*)client->fOutputList->mBuffers[i].mData, nframes * sizeof(float));
								firstproc[i] = false;
							} else {			// other proc : mix
								//JARLog("Process GetStreamUsage NO other proc : mix = %ld\n", proc->GetIOProc());
								for (UInt32 j = 0; j < nframes; j++) {
									output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
								}
							}
						} else {
							//JARLog("JackRouterDevice::Process client->fOutputPortList[i] %d is null\n",i); 
						}
						
                    }
                }
			}
		}
		 
		// Final Unlock
		client->mIOGuard.Unlock();
    } else {
	
		// No IOProc so clear buffers
		
		//JARLog("No IOProc \n");
		for (int i = 0; i < JackRouterDevice::fOutputChannels; i++) {
			if (client->fOutputPortList[i]) {
				float* output = (float*)jack_port_get_buffer(client->fOutputPortList[i], nframes);
				memset(output, 0, nframes * sizeof(float));
			}
		}
		
		// Final Unlock
		client->mIOGuard.Unlock();
	}

    map<int, pair<float*, jack_port_t*> >::const_iterator it;

    // Copy temp buffers from VST plug-ins into the Jack buffers
    for (it = client->fPlugInPortsVST.begin(); it != client->fPlugInPortsVST.end(); it++) {
        pair<float*, jack_port_t*> obj = it->second;
        memcpy((float*)jack_port_get_buffer(obj.second, nframes), obj.first, nframes * sizeof(float));
        memset(obj.first, 0, nframes * sizeof(float));
    }
	
    // Copy temp buffers from AU plug-ins into the Jack buffers
    for (it = client->fPlugInPortsAU.begin(); it != client->fPlugInPortsAU.end(); it++) {
        pair<float*, jack_port_t*> obj = it->second;
        memcpy((float*)jack_port_get_buffer(obj.second, nframes), obj.first, nframes * sizeof(float));
        memset(obj.first, 0, nframes * sizeof(float));
    }

	return 0;
}

bool JackRouterDevice::Open()
{
	pid_t pid = getpid();
	jack_options_t options = JackNullOption;
	jack_status_t status;
    char* id_name = bequite_getNameFromPid(pid);
    JARLog("JackRouterDevice::Open id %ld name %s\n", pid, id_name);
    assert(id_name != NULL);
	
	// From previous state
	if (fClient) {
		JARLog("Close old client\n");
		jack_client_close(fClient);
		mIOProcList->RemoveAllIOProcs();
	}

    if ((fClient = jack_client_open(id_name, options, &status)) == NULL) {
        JARLog("JackRouterDevice::Open jack server not running?\n");
        return false;
    } else {
		jack_set_process_callback(fClient, Process, this);
		jack_on_shutdown(fClient, Shutdown, this);
		jack_set_buffer_size_callback(fClient, BufferSize, this);
		jack_set_xrun_callback(fClient, XRun, this);
		return true;
	}
}

void JackRouterDevice::Close()
{
    JARLog("Close\n");

    if (fClient) {
        if (jack_client_close(fClient) != 0) {
            JARLog("JackRouterDevice::Close cannot close client\n");
        }
		fClient = NULL;
    }
}

void JackRouterDevice::Destroy()
{
    JARLog("Close\n");

    if (fClient) {
        if (jack_client_close(fClient) != 0) {
            JARLog("JackRouterDevice::Close cannot close client\n");
        }
		fClient = NULL;
    }
	
	mIOProcList->RemoveAllIOProcs();
	fFirstActivate = true;
		
	StopHardware();
}

int JackRouterDevice::BufferSize(jack_nframes_t nframes, void* arg)
{
    JackRouterDevice* client = (JackRouterDevice*)arg;
    JARLog("New BufferSize %ld\n", nframes);

    JackRouterDevice::fBufferSize = nframes;

    for (long i = 0; i < JackRouterDevice::fInputChannels; i++) {
        client->fInputList->mBuffers[i].mNumberChannels = 1;
        client->fInputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
    }

    for (long i = 0; i < JackRouterDevice::fOutputChannels; i++) {
        client->fOutputList->mBuffers[i].mNumberChannels = 1;
        client->fOutputList->mBuffers[i].mDataByteSize = JackRouterDevice::fBufferSize * sizeof(float);
        free(client->fOuputListTemp[i]);
        client->fOuputListTemp[i] = (float*)malloc(sizeof(float) * JackRouterDevice::fBufferSize);
    }
	
	CAPropertyAddress thePropertyBufferFrameSize(kAudioDevicePropertyBufferFrameSize);
	client->PropertiesChanged(1, &thePropertyBufferFrameSize);
	CAPropertyAddress thePropertyBufferFrameSizeAddress(kAudioDevicePropertyBufferFrameSize);
	client->PropertiesChanged(1, &thePropertyBufferFrameSizeAddress);
    return 0;
}

void JackRouterDevice::Shutdown(void* /*arg */)
{}

int JackRouterDevice::XRun(void* arg)
{
    JARLog("XRun\n");
	JackRouterDevice* client = (JackRouterDevice*)arg;
	CAPropertyAddress theProcessorOverloadAddress(kAudioDeviceProcessorOverload);
	client->PropertiesChanged(1, &theProcessorOverloadAddress);
    return 0;
}

bool JackRouterDevice::Activate()
{
    JARLog("Activate\n");

    if (jack_activate(fClient)) {
        JARLog("cannot activate client");
        return false;
    } else {
        // Autoconnect is only done for the first activation
        if (JackRouterDevice::fFirstActivate) {
            JARLog("First activate\n");
            AutoConnect();
            JackRouterDevice::fFirstActivate = false;
        } else {
            RestoreConnections();
        }
        return true;
    }
}

bool JackRouterDevice::Desactivate()
{
    JARLog("Desactivate\n");
	SaveConnections();

    if (jack_deactivate(fClient)) {
        JARLog("cannot deactivate client\n");
        return false;
    }
    return true;
}

bool JackRouterDevice::AllocatePlugInPortVST(int num)
{
    JARLog("AllocatePlugInPortVST %ld\n", num + 1);
    char name[256];
    sprintf(name, "VSTsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, fBufferSize * sizeof(float));
    fPlugInPortsVST[num] = make_pair(buffer, port);
    return true;
}

bool JackRouterDevice::AllocatePlugInPortAU(int num)
{
    JARLog("AllocatePlugInPortAU %ld\n", num + 1);
    char name[256];
    sprintf(name, "AUsend%d", num + 1);
    jack_port_t* port = jack_port_register(fClient, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;

    float* buffer = (float*)malloc(sizeof(float) * fBufferSize);
    if (!buffer)
        return false;

    memset(buffer, 0, fBufferSize * sizeof(float));
    fPlugInPortsAU[num] = make_pair(buffer, port);
    return true;
}

float* JackRouterDevice::GetPlugInPortVST(int num)
{
    JARLog("GetPlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    return obj.first;
}

float* JackRouterDevice::GetPlugInPortAU(int num)
{
    JARLog("GetPlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    return obj.first;
}

void JackRouterDevice::ReleasePlugInPortVST(int num)
{
    JARLog("ReleasePlugInPortVST %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsVST[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsVST.erase(num); /// TO CHECK : RT access ??
}

void JackRouterDevice::ReleasePlugInPortAU(int num)
{
    JARLog("ReleasePlugInPortAU %ld\n", num);
    pair<float*, jack_port_t*> obj = fPlugInPortsAU[num];
    assert(obj.first);
    assert(obj.second);
    free(obj.first);
    jack_port_unregister(fClient, obj.second);
    fPlugInPortsAU.erase(num); /// TO CHECK : RT access ??
}



