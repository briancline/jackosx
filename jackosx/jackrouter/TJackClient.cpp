/*
  Copyright © Stefan Werner stefan@keindesign.de, Grame 2003,2004

  This library is free software; you can redistribute it and modify it under
  the terms of the GNU Library General Public License as published by the
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  grame@rd.grame.fr

*/

/*
History

14-08-03 : First version : S Letz

15-08-03 : Version 0.05 : S Letz
        DeviceAddIOProc : check if a client is already setup, 
        Improve running management. Implement kAudioDevicePropertyPreferredChannelsForStereo.
        Cleanup error codes.
        
16-08-03 : Version 0.06 : S Letz
        Experimental : special Property "kAudioDevicePropertyJackClient" to access Jack client from application code.
        fInputChannels are now allocated for Input, fOutputChannels are now allocated for Ouput ==> this JAS can be selected
        as default input/output in Audio/Midi setup.
 
21-08-03 : Version 0.07 : S Letz
        Completly implement DeviceGetPropertyInfo and StreamGetPropertyInfo : to be checked
    
02-09-03 : Version 0.08 : S Letz
        Correct kAudioDevicePropertyStreamFormatSupported implementation, add Johnny preferences file management code.

03-09-03 : Version 0.09 : S Letz
        Improves stream management, now Audio/Midi setup and Daisy correctly display streams settings.
        Vokator works correctly at launch time, but still does not work when changing audio settings dynamically.
        
03-09-03 : Version 0.10 : Johnny Petrantoni
        Now the names of the clients are the names of the applications that is using JAS. (procinfo folder).
        
03-09-03 : Version 0.11 : S Letz
        Change the way the Jack client is activated. Now QuickTime Player runs correctly.
        
04-09-03 : Version 0.12 : S Letz
        Add kLinearPCMFormatFlagIsFloat in stream format. Implement kAudioDevicePropertyStreamFormatMatch.
        
04-09-03 : Version 0.13 : S Letz
        Correct kAudioDevicePropertyPreferredChannelsForStereo management. Now QuickTime Player seems to play on the correct channels.

05-09-03 : Version 0.14 : S Letz
        Redirect some calls to the Jack CoreAudio driver. Warning : hard coded CoreAudio driver (Built-in).
        Still problems with clock sources....

08-09-03 : Version 0.15 : S Letz
        Redirect some calls to the Jack CoreAudio driver. Now get the Jack CoreAudio driver name dynamically.
        Still problems with clock sources....   
      
09-09-03 : Version 0.16 : S Letz
        Implement AudioTimeStamp on Process, implement DeviceGetCurrentTime, call listener in Start and Stop (for Reason) 

10-09-03 : Version 0.17 : S Letz
        Redirect some calls to the Jack CoreAudio driver : the same calls are redirected in DeviceGetPropertyInfo and DeviceGetProperty. 
		Clock sources problems seem to be solved.

10-09-03 : Version 0.18 : Johnny Petrantoni
        Correct ReadPref.

10-09-03 : Version 0.19 : S Letz
        Check QueryDevice return code in Initialize. Add streams and device desallocation in Teardown.

11-09-03 : Version 0.20 : S Letz
        Redirect all still unhandled calls to the Jack CoreAudio driver.

11-09-03 : Version 0.21 : S Letz
        Implements kAudioDevicePropertyDeviceName and all in StreamGetPropertry (DP4 use it...), Implements kAudioStreamPropertyOwningDevice. 	
		Correct some extra property management issues.

12-09-03 : Version 0.22 : S Letz
        QueryDevices : correct bug in Jack CoreAudio driver name extraction.

12-09-03 : Version 0.23 : S Letz
        Support for multiple IOProc, implements mixing.

13-09-03 : Version 0.24 : S Letz
        More explicit debug messages.

14-09-03 : Version 0.25 : S Letz
        Implement kAudioDevicePropertyUsesVariableBufferFrameSizes in DeviceGetProperty.
       
15-09-03 : Version 0.26 : S Letz
        Correct bug in multiple IOProc management.
       
16-09-03 : Version 0.27 : S Letz
        Return kAudioHardwareUnknownPropertyError in case of unknown property.
       
16-09-03 : Version 0.28 : S Letz
        Correct DeviceGetProperty for kAudioDevicePropertyDeviceIsAlive (back to old behaviour).
       
22-09-03 : Version 0.29 : S Letz
        Improves kAudioStreamPropertyTerminalType management.
        Correct stream flags configuration. Should not use kAudioFormatFlagIsNonInterleaved !!
        Now iTunes, Reason and DLSynth are working...
       
22-09-03 : Version 0.30 : S Letz
        Implements GetProperty for kAudioDevicePropertyDataSource, kAudioDevicePropertyDataSources,
        kAudioDevicePropertyDataSourceNameForID and kAudioDevicePropertyDataSourceNameForIDCFString.
               
23-09-03 : Version 0.31 : S Letz
        Change again JackFlags value... kAudioFormatFlagIsBigEndian was the lacking one that made iTumes and other not work..
        kAudioFormatFlagIsNonInterleaved is back again.
       
23-09-03 : Version 0.32 : S Letz
        Client side more robust error handling when the Jackd server quits. Use the jack_on_shutdown callback to cleany deactivate
        the JAS client. Quitting the server, changing it's settings, reactivating the clients should work.
        
26-09-03 : Version 0.33 : Johnny Petrantoni
        New ReadPref.

29-09-03 : Version 0.34 : S Letz
        Better version management.

01-10-03 : Version 0.35 : S Letz
        Code cleanup. kAudioDevicePropertyJackClient now opens the Jack client if it is not already running (needed for VST plugins)

13-10-03 : Version 0.36 : S Letz
        Major code cleanup. kAudioDevicePropertyJackClient renamed to kAudioDevicePropertyGetJackClient. New kAudioDevicePropertyReleaseJackClient
        to release the jack client.

14-10-03 : Version 0.37 : S Letz, Johnny Petrantoni
        Correct kAudioDevicePropertyStreamFormatMatch property. Remove the fixed size length for CoreAudio driver names.
        Correct driver name and manufacturer string length bug. Correct kAudioDevicePropertyStreamConfiguration bug.
        Cleanup error codes. Correct a bug in Activate method when connecting jack ports.

18-10-03 : Version 0.38 : S Letz
        Removes unused code.
        
19-10-03 : Version 0.39 : S Letz
        Improve DeviceStart/DeviceStop : now each proc use a running status. Check for multiple start/stop. Check for multiple same IOProc. 	
		Correct bugs in DeviceGetProperty, ioPropertyDataSize was not correctly set.

19-10-03 : Version 0.40 : S Letz
        Check HAL connection in Teardown. More debug messages. Cleanup ReadPref.
        
22-10-03 : Version 0.41 : S Letz
        Checking of parameter size value in DeviceGetProperty. Management of outWritable in DeviceGetPropertyInfo and
        StreamGetPropertyInfo.
        
23-10-03 : Version 0.42 : S Letz
        Correct a bug that cause crash when JASBus is used without JAS activated. Prepare the code for compilation with QT 6.4.

28-10-03 : Version 0.43 : S Letz
        Cleanup.

03-11-03 : Version 0.44 : Johnny Petrantoni. Implement kAudioDevicePropertyPreferredChannelLayout on Panther.

08-11-03 : Version 0.45 : S Letz 
		kAudioDevicePropertyUsesVariableBufferFrameSizes not supported in DeviceGetPropertyInfo. 
		Return a result in DeviceGetPropertyInfo even when outSize is null.
		Change management of kAudioDevicePropertyDataSource and kAudioDevicePropertyDataSources.
		Idea for Panther default system driver problem. Update ReadPref.         

09-01-04 : Version 0.46 : S Letz 
		Ugly hack to solve the iTunes deconnection problem.
		  
11-01-04 : Version 0.47 : S Letz 
		Removes ugly hack, implement SaveConnections and RestoreConnections (to solve the iTunes deconnection problem),
		Unregister jack port in Close. Check jack_port_register result.	Return kAudioHardwareUnsupportedOperationError
		for unsupported DeviceRead, DeviceGetNearestStartTime and DeviceTranslateTime.	
		
15-01-04 : Version 0.48 : S Letz 
		Implement null proc behaviour in DeviceStop (to be checked)
		Implement (outPropertyData == NULL) && (ioPropertyDataSize != NULL) case in DeviceGetPropertyInfo and StreamGetPropertyInfo
        In process, clean the ouput buffers in all cases to avoid looping an old buffer in some situations.
		Implement null proc behaviour in DeviceStart and DeviceStop.

16-01-04 : Version 0.49 : S Letz
        Rename flags kJackStreamFormat and kAudioTimeFlags.
		
22-01-04 : Version 0.50 : S Letz
        Rename Jack Audio Server to Jack Router.

24-01-04 : Version 0.51 : S Letz  Johnny Petrantoni
        Implement kAudioDevicePropertyIOProcStreamUsage. Implement kAudioDevicePropertyUsesVariableBufferFrameSizes in in DeviceGetProperty.
        This solve the iMovie 3.03 crash. Improve debug code using Johnny's code.  Reject "jackd" as a possible client.
		Fixed ReadPref bug introduced when improving Debug code. Add kAudioHardwarePropertyBootChimeVolumeScalar in DeviceGetPropertyInfo.
		Correct bug in CheckServer.
		
12-07-04 : Version 0.52 : S Letz
		Check TJackClient::fJackClient in SetProperty.

13-07-04 : Version 0.53 : S Letz
		Correct bug in kAudioDevicePropertyIOProcStreamUsage management.
	
1-09-04 : Version 0.54 : S Letz
		Distinguish kAudioDevicePropertyActualSampleRate and kAudioDevicePropertyNominalSampleRate.
		kAudioDevicePropertyActualSampleRate is returned only when the driver is running.

1-09-04 : Version 0.55 : S Letz
		Correct use of AudioHardwareStreamsCreated and AudioHardwareStreamsDied functions (they are using an array of streamIDs)

10-09-04 : Version 0.56 : S Letz
		Correct DeviceSetProperty: some properties can be "set" event there is no Jack client running other like kAudioDevicePropertyIOProcStreamUsage not.

14-09-04 : Version 0.57 : S Letz
		Improve the external (Jack plugins) management : now internal and external "clients" are distinguished. Correct Save/Restore connections bug.
		
03-10-04 : Version 0.58 : S Letz
		Correct bug in bequite_getNameFromPid. This solve the Band-in-a Box bug. 
		New KillJackClient method to be called in TearDown when clients do not correctly quit (like iMovie).
		 
TODO :
    
        - solve zombification problem of Jack (remove time-out check or use -T option)
        - check kAudioDevicePropertyPreferredChannelLayout parameters.
        ....

*/

#include "TJackClient.h"
#include <stdio.h>
#include <assert.h>

// Static variables declaration

long TJackClient::fBufferSize;
float TJackClient::fSampleRate;

long TJackClient::fInputChannels = 0;
long TJackClient::fOutputChannels = 0;

bool TJackClient::fAutoConnect = true;
bool TJackClient::fDeviceRunning = false;
bool TJackClient::fConnected2HAL = false;
bool TJackClient::fDefaultInput = true;
bool TJackClient::fDefaultOutput = true;
bool TJackClient::fDefaultSystem = true;
list<pair<string,string> > TJackClient::fConnections; 

string TJackClient::fDeviceName = "Jack Router";
string TJackClient::fStreamName = "Float32";
string TJackClient::fDeviceManufacturer = "Grame";

AudioDeviceID TJackClient::fDeviceID;
TJackClient*  TJackClient::fJackClient = NULL;

AudioStreamID TJackClient::fStreamIDList[MAX_JACK_PORTS]; 
AudioStreamID TJackClient::fCoreAudioDriver = 0;
AudioHardwarePlugInRef TJackClient::fPlugInRef = 0;

#define kJackStreamFormat kAudioFormatFlagIsPacked|kLinearPCMFormatFlagIsFloat|kAudioFormatFlagIsBigEndian|kAudioFormatFlagIsNonInterleaved

#define kAudioTimeFlags kAudioTimeStampSampleTimeValid|kAudioTimeStampHostTimeValid|kAudioTimeStampRateScalarValid

struct stereoList
{
	UInt32	channel[2];
};
typedef struct stereoList stereoList;

#define PRINTDEBUG 1

//------------------------------------------------------------------------
static void JARLog(char *fmt,...) 
{
#ifdef PRINTDEBUG
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"JAR: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

//------------------------------------------------------------------------
static void Print4CharCode(char* msg, long c)	{ 
#ifdef PRINTDEBUG 
	UInt32 __4CC_number = (c);		
	char __4CC_string[5];		
	memcpy(__4CC_string, &__4CC_number, 4);	
	__4CC_string[4] = 0;			
	JARLog("%s'%s'\n", (msg), __4CC_string);
#endif
}
						
//------------------------------------------------------------------------
static void printError(OSStatus err) 
{
#ifdef PRINTDEBUG
    switch (err) {
        case kAudioHardwareNoError:
            JARLog("error code : kAudioHardwareNoError\n");
            break;
         case kAudioHardwareNotRunningError:
            JARLog("error code : kAudioHardwareNotRunningError\n");
            break;
        case kAudioHardwareUnspecifiedError:
            printf("error code : kAudioHardwareUnspecifiedError\n");
        case kAudioHardwareUnknownPropertyError:
            JARLog("error code : kAudioHardwareUnknownPropertyError\n");
            break;
        case kAudioHardwareBadPropertySizeError:
            JARLog("error code : kAudioHardwareBadPropertySizeError\n");
            break;
        case kAudioHardwareIllegalOperationError:
            JARLog("error code : kAudioHardwareIllegalOperationError\n");
            break;
        case kAudioHardwareBadDeviceError:
            JARLog("error code : kAudioHardwareBadDeviceError\n");
            break;
        case kAudioHardwareBadStreamError:
            JARLog("error code : kAudioHardwareBadStreamError\n");
            break;
        case kAudioDeviceUnsupportedFormatError:
            JARLog("error code : kAudioDeviceUnsupportedFormatError\n");
            break;
      	case kAudioDevicePermissionsError:
            JARLog("error code : kAudioDevicePermissionsError\n");
            break;
        default:
            JARLog("error code : unknown\n");
            break;
    }
#endif
}

//------------------------------------------------------------------------
void TJackClient::SaveConnections()
{
    const char **ports,**connections;
    
    if (!fClient) return;
    
	JARLog("--------------------------------------------------------\n");
    JARLog("SaveConnections\n");

    fConnections.clear();
  
    for (int i = 0; i < TJackClient::fInputChannels; ++i) {
       if (fInputPortList[i] && (connections = jack_port_get_connections(fInputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(connections[j],jack_port_name(fInputPortList[i])));
            }
            free(connections);
        } 
    }

    for (int i = 0; i < TJackClient::fOutputChannels; ++i) {
        if (fOutputPortList[i] && (connections = jack_port_get_connections(fOutputPortList[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(jack_port_name(fOutputPortList[i]),connections[j]));
			}
            free(connections);
        } 
    }

#if PRINTDEBUG    
    list<pair<string,string> >::const_iterator it;
    
    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string,string> connection = *it;
        JARLog("connections : %s %s\n", connection.first.c_str(),connection.second.c_str());
    }
#endif
}

//------------------------------------------------------------------------
void TJackClient::RestoreConnections()
{
	JARLog("--------------------------------------------------------\n");
    JARLog("RestoreConnections\n");

    list<pair<string,string> >::const_iterator it;
    
    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string,string> connection = *it;
		JARLog("connections : %s %s\n", connection.first.c_str(),connection.second.c_str());
        jack_connect(fClient,connection.first.c_str(),connection.second.c_str());
    }
    
    fConnections.clear();
}

//------------------------------------------------------------------------
TJackClient* TJackClient::GetJackClient() 
{
    JARLog("GetJackClient\n");

    if (TJackClient::fJackClient) { 
        return TJackClient::fJackClient;
    }else{
        TJackClient::fJackClient = new TJackClient();

        if (TJackClient::fJackClient->Open()) {
            return TJackClient::fJackClient;
        }else{
            delete TJackClient::fJackClient;
            TJackClient::fJackClient = NULL;
            return NULL;
        }
    }
}

//------------------------------------------------------------------------
void TJackClient::ClearJackClient()
{
    JARLog("ClearJackClient\n");

    if (TJackClient::fJackClient) {
        TJackClient::fJackClient->ClearIOProc();
        delete TJackClient::fJackClient;
        TJackClient::fJackClient = NULL;
    }
}

//------------------------------------------------------------------------
void TJackClient::KillJackClient()
{
   if (TJackClient::fJackClient) {
		TJackClient::fJackClient->Desactivate();
	    TJackClient::fJackClient->Close();
        delete TJackClient::fJackClient;
        TJackClient::fJackClient = NULL;
    }
}

//------------------------------------------------------------------------
void TJackClient::CheckLastRef()
{
    assert(TJackClient::fJackClient);
    
    if (TJackClient::fJackClient->fExternalClientNum + TJackClient::fJackClient->fInternalClientNum == 0) {
   	    JARLog("DeviceRemoveIOProc : last proc, remove client\n");
 	#if PRINTDEBUG
        bool res = TJackClient::fJackClient->Desactivate();
		if (!res) JARLog("cannot desactivate client\n");
	#else
		TJackClient::fJackClient->Desactivate();
	#endif
        TJackClient::fJackClient->Close();
        delete TJackClient::fJackClient;
        TJackClient::fJackClient = NULL;
    }
}

//------------------------------------------------------------------------
void TJackClient::IncRefInternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fInternalClientNum++;
	JARLog("IncRefInternal : %ld\n",TJackClient::fJackClient->fInternalClientNum);
	if (TJackClient::fJackClient->fInternalClientNum == 1) 
		TJackClient::fJackClient->AllocatePorts();
	TJackClient::fJackClient->Activate();  
}

//------------------------------------------------------------------------
void TJackClient::DecRefInternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fInternalClientNum--;
    JARLog("DecRefInternal : %ld\n",TJackClient::fJackClient->fInternalClientNum);
	if (TJackClient::fJackClient->fInternalClientNum == 0) 
		TJackClient::fJackClient->DisposePorts();
    CheckLastRef();
}

//------------------------------------------------------------------------
void TJackClient::IncRefExternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fExternalClientNum++;
	JARLog("IncRefExternal : %ld\n",TJackClient::fJackClient->fExternalClientNum);
	TJackClient::fJackClient->Activate();
}

//------------------------------------------------------------------------
void TJackClient::DecRefExternal()
{
    assert(TJackClient::fJackClient);
    TJackClient::fJackClient->fExternalClientNum--;
    JARLog("DecRefExternal : %ld\n",TJackClient::fJackClient->fExternalClientNum);
	CheckLastRef();
}

//------------------------------------------------------------------------
void TJackClient::Shutdown (void *arg)
{
    TJackClient::fDeviceRunning = false;
    JARLog("Shutdown\n");
    ClearJackClient();
	OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                        TJackClient::fDeviceID, 
                                                        0, 
                                                        0, 
                                                        kAudioDevicePropertyDeviceIsAlive);
    JARLog("Shutdown err %ld\n",err);
}

//---------------------------------------------------------------------------
void TJackClient::SetTime(AudioTimeStamp* timeVal, long curTime, UInt64 time)
{
	timeVal->mSampleTime = curTime;
    timeVal->mHostTime = time;
    timeVal->mRateScalar = 1.0;
    timeVal->mWordClockTime = 0;
    timeVal->mFlags = kAudioTimeFlags;
}

/* Jack Process callback */
//------------------------------------------------------------------------
int TJackClient::Process(jack_nframes_t nframes, void *arg)
{
    OSStatus err;
    AudioTimeStamp inNow;
    AudioTimeStamp inInputTime;
    AudioTimeStamp inOutputTime;
    
    TJackClient* client = (TJackClient*)arg;
    UInt64 time = AudioGetCurrentHostTime();
    long curTime = client->IncTime();
    
	SetTime(&inNow,curTime,time);
	SetTime(&inInputTime,curTime-TJackClient::fBufferSize,time);
	SetTime(&inOutputTime,curTime+TJackClient::fBufferSize,time);
      
    // One IOProc
    if (client->GetProcNum() == 1) {
	
		// Clear output ports in all cases
		for (int i = 0; i<TJackClient::fOutputChannels; i++) {
			memset((float *)jack_port_get_buffer(client->fOutputPortList[i], nframes), 0, nframes*sizeof(float));
		}
   
        pair<AudioDeviceIOProc,TProcContext> val = *client->fAudioIOProcList.begin();
        TProcContext context = val.second;
       
        if (context.fStatus) { // If proc is started
		
			if (context.fStreamUsage) {
			
				// Only set up buffers that are really needed
				for (int i = 0; i<TJackClient::fInputChannels; i++) {
					if (context.fInput[i]) {
						client->fInputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fInputPortList[i], nframes);
					}else{
						client->fInputList->mBuffers[i].mData = NULL;
					}
				}
				
				for (int i = 0; i<TJackClient::fOutputChannels; i++) {
					if (context.fOutput[i]) {
						client->fOutputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fOutputPortList[i], nframes);
					}else{
						client->fOutputList->mBuffers[i].mData = NULL;
					}
				}
			
			}else{
				// Non Interleaved 
				for (int i = 0; i<TJackClient::fInputChannels; i++) {
					client->fInputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fInputPortList[i], nframes);
				}
				
				for (int i = 0; i<TJackClient::fOutputChannels; i++) {
					client->fOutputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fOutputPortList[i], nframes);
				}
            }
			
			err = (val.first) (client->fDeviceID, 
                                &inNow, 
                                client->fInputList, 
                                &inInputTime, 
                                client->fOutputList, 
                                &inOutputTime, 
                                context.fContext);
		                         
        #if PRINTDEBUG
            if (err != kAudioHardwareNoError) JARLog("Process error %ld\n", err);
        #endif
        }
                  
    }else if (client->GetProcNum() > 1) { // Several IOProc : need mixing  
	
		for (int i = 0; i<TJackClient::fOutputChannels; i++) {
			memset((float *)jack_port_get_buffer(client->fOutputPortList[i], nframes), 0, nframes*sizeof(float));
            // Use an intermediate mixing buffer
            memset(client->fOuputListMixing[i], 0, nframes*sizeof(float));
		}
    
        map<AudioDeviceIOProc,TProcContext>::iterator iter;
        for (iter = client->fAudioIOProcList.begin(); iter != client->fAudioIOProcList.end(); iter++) {
        
            pair<AudioDeviceIOProc,TProcContext> val = *iter;
            TProcContext context = val.second;
            
            if (context.fStatus) { // If proc is started
			
				if (context.fStreamUsage) {
						
					// Only set up buffers that are really needed
					for (int i = 0; i<TJackClient::fInputChannels; i++) {
						if (context.fInput[i]) {
							client->fInputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fInputPortList[i], nframes);
						}else{
							client->fInputList->mBuffers[i].mData = NULL;
						}
					}
					 
					for (int i = 0; i<TJackClient::fOutputChannels; i++) {
						// Use an intermediate mixing buffer
						if (context.fOutput[i]) {
							client->fOutputList->mBuffers[i].mData = client->fOuputListMixing[i];
						}else{
							client->fOutputList->mBuffers[i].mData = NULL;
						}
					}
				
				}else {
				
					for (int i = 0; i<TJackClient::fInputChannels; i++) {
						client->fInputList->mBuffers[i].mData = (float *)jack_port_get_buffer(client->fInputPortList[i], nframes);
					}
					
					for (int i = 0; i<TJackClient::fOutputChannels; i++) {
						// Use an intermediate mixing buffer
						client->fOutputList->mBuffers[i].mData = client->fOuputListMixing[i];
					}
				}
            
                err = (val.first) (client->fDeviceID, 
                                    &inNow, 
                                    client->fInputList, 
                                    &inInputTime, 
                                    client->fOutputList, 
                                    &inOutputTime, 
                                    context.fContext);
		                                     
            #if PRINTDEBUG
                if (err != kAudioHardwareNoError) JARLog("Process error %ld\n", err);
            #endif
			
				// Only mix buffers that are really needed
                if (context.fStreamUsage) {
					for (int i = 0; i<TJackClient::fOutputChannels; i++) {
						if (context.fOutput[i]) {
							float * output = (float *)jack_port_get_buffer(client->fOutputPortList[i], nframes);
							for (int j = 0; j<nframes; j++) {
								output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
							}
						}
					}
				
				}else{
					for (int i = 0; i<TJackClient::fOutputChannels; i++) {
						float * output = (float *)jack_port_get_buffer(client->fOutputPortList[i], nframes);
						for (int j = 0; j<nframes; j++) {
							output[j] += ((float*)client->fOutputList->mBuffers[i].mData)[j];
						}
					}
				}
            }
        }
    }
     
    return 0;
}    

//------------------------------------------------------------------------
TJackClient::TJackClient()
{
    JARLog("TJackClient constructor\n");

    fInputList = (AudioBufferList*)malloc(sizeof(UInt32)+sizeof(AudioBuffer)*TJackClient::fInputChannels);
    assert(fInputList);
    fOutputList = (AudioBufferList*)malloc(sizeof(UInt32)+sizeof(AudioBuffer)*TJackClient::fOutputChannels);
    assert(fOutputList);
    
    fInputList->mNumberBuffers = TJackClient::fInputChannels;
    fOutputList->mNumberBuffers = TJackClient::fOutputChannels;
    
    fOuputListMixing = (float **)malloc(sizeof(float*)*TJackClient::fOutputChannels);
    assert(fOuputListMixing);
    
    for (int i = 0; i < TJackClient::fOutputChannels; i++) {
        fOuputListMixing[i] = (float *)malloc(sizeof(float)*TJackClient::fBufferSize);
        assert(fOuputListMixing[i]);
    }
	 
    for (int i = 0; i < MAX_JACK_PORTS; i++) {
		fInputPortList[i] = 0;
		fOutputPortList[i] = 0;
    }
    
    fProcRunning = 0;
    fSampleTime = 0;
    fExternalClientNum = 0;
	fInternalClientNum = 0;
}

//------------------------------------------------------------------------
TJackClient::~TJackClient()
{
    JARLog("TJackClient destructor\n");

    free(fInputList);
    free(fOutputList);
    
    for (int i = 0; i<TJackClient::fOutputChannels; i++) free(fOuputListMixing[i]);
    free(fOuputListMixing);
}
           
//------------------------------------------------------------------------
bool TJackClient::AllocatePorts()
{
    char in_port_name [JACK_PORT_NAME_LEN];
    
    for (long i = 0; i<TJackClient::fInputChannels; i++) {
        sprintf(in_port_name, "in%ld", i+1);
        if ((fInputPortList[i] = jack_port_register(fClient, in_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == NULL) 
			 goto error;
		fInputList->mBuffers[i].mNumberChannels = 1;
        fInputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize*sizeof(float);
    }
    
    char out_port_name [JACK_PORT_NAME_LEN];
    
    for (long i = 0; i<TJackClient::fOutputChannels; i++) {
        sprintf(out_port_name, "out%ld", i+1);
		if ((fOutputPortList[i] = jack_port_register(fClient, out_port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL) 
			goto error;
        fOutputList->mBuffers[i].mNumberChannels = 1;
        fOutputList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize*sizeof(float);
    }

    return true;
     
error:

	JARLog("Cannot register ports\n");
	DisposePorts();
    return false;
}

//------------------------------------------------------------------------
void TJackClient::DisposePorts()
{
    JARLog("DisposePorts\n");
	SaveConnections();

	for (long i = 0; i<TJackClient::fInputChannels; i++) {
		if (fInputPortList[i]) {
			jack_port_unregister(fClient, fInputPortList[i]);
			fInputPortList[i] = 0;
		}
    }
    
	for (long i = 0; i<TJackClient::fOutputChannels; i++) {
        if (fOutputPortList[i]) {
			jack_port_unregister(fClient, fOutputPortList[i]);
			fOutputPortList[i] = 0;
		}
    }
}

//------------------------------------------------------------------------
bool TJackClient::Open()
{
    char* id_name = bequite_getNameFromPid((int)getpid()); 
    
    if ((fClient = jack_client_new(id_name)) == NULL) {
		JARLog("jack server not running?\n");
		goto error;
    }
          
    jack_set_process_callback(fClient, Process, this);
    jack_on_shutdown(fClient, Shutdown, NULL);
    return true;
     
error:
    return false;
}

//------------------------------------------------------------------------
void TJackClient::Close()
{
    JARLog("Close\n");

	//DisposePorts();
	
	if (fClient) {
		if (jack_client_close(fClient)) {
			JARLog("Cannot close client\n");
		}
	}
}

//------------------------------------------------------------------------
bool TJackClient::AddIOProc(AudioDeviceIOProc proc, void* context) 
{
    if (fAudioIOProcList.find(proc) == fAudioIOProcList.end()) {
        fAudioIOProcList.insert(make_pair(proc,TProcContext(context)));
        JARLog("AddIOProc fAudioIOProcList.size %ld \n",fAudioIOProcList.size());
        return true;
    }else{
        JARLog("AddIOProc proc already added %x \n",proc);
        return false;
    }
}

//------------------------------------------------------------------------
bool TJackClient::RemoveIOProc(AudioDeviceIOProc proc)
{
    if (fAudioIOProcList.find(proc) != fAudioIOProcList.end()) {
        fAudioIOProcList.erase(proc);
        JARLog("fAudioIOProcList size %ld \n",fAudioIOProcList.size());
        return true;
    }else{
        JARLog("RemoveIOProc proc not present %x \n",proc);
        return false;
    }
}

//------------------------------------------------------------------------
void TJackClient::Start(AudioDeviceIOProc proc) 
{
	if (proc == NULL) { // is supposed to start the hardware
		IncRunning();
		OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
															TJackClient::fDeviceID, 
															0, 
															0, 
															kAudioDevicePropertyDeviceIsRunning);
	}else{
		map<AudioDeviceIOProc,TProcContext>::iterator iter = fAudioIOProcList.find(proc);
		if (iter != fAudioIOProcList.end()) {
			if (!iter->second.fStatus) { // check multiple start of the proc
				iter->second.fStatus = true;
				IncRunning();
				OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
																TJackClient::fDeviceID, 
																0, 
																0, 
																kAudioDevicePropertyDeviceIsRunning);
				JARLog("TJackClient::Start err %ld\n",err);
	
			}else{
				JARLog("TJackClient::Start proc already started %x\n",proc);
			}
		}else{
			JARLog("Start proc %x not found\n",proc);
		}
	}
}

//------------------------------------------------------------------------
void TJackClient::Stop(AudioDeviceIOProc proc) 
{
	if (proc == NULL) { // is supposed to stop the hardware
		StopRunning();
		OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
															TJackClient::fDeviceID, 
															0, 
															0, 
															kAudioDevicePropertyDeviceIsRunning);
	}else{
		map<AudioDeviceIOProc,TProcContext>::iterator iter = fAudioIOProcList.find(proc);
		if (iter != fAudioIOProcList.end()) {
			if (iter->second.fStatus) { // check multiple stop of the proc
				iter->second.fStatus = false;
				DecRunning();
				OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
																TJackClient::fDeviceID, 
																0, 
																0, 
																kAudioDevicePropertyDeviceIsRunning);
				JARLog("TJackClient::Stop err %ld\n",err);
			
			}else{
				JARLog("TJackClient::Stop proc already stopped %x\n",proc);
			}        
		}else{
			JARLog("Stop : proc %x not found\n",proc);
		}
	}
}

//------------------------------------------------------------------------
bool TJackClient::Activate()
{
    const char **ports;
    
    if (jack_activate(fClient)) {
		JARLog("cannot activate client");
		goto error;
    }
    
    if (TJackClient::fAutoConnect) {
    
        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
			JARLog("cannot find any physical capture ports\n");
        }else{
            
            for (int i = 0; i<TJackClient::fInputChannels; i++) {
                #if PRINTDEBUG
                    if (ports[i]) JARLog("ports[i] %s\n",ports[i]);
                    if (fInputPortList[i] && jack_port_name(fInputPortList[i])) 
                        JARLog("jack_port_name(fInputPortList[i]) %s\n",jack_port_name(fInputPortList[i]));
                #endif
                
                // Stop condition
                if (ports[i] == 0) break;
				
				if (fInputPortList[i] && jack_port_name(fInputPortList[i])) {
					if (jack_connect(fClient, ports[i], jack_port_name(fInputPortList[i]))) {
						JARLog("cannot connect input ports\n");
					}
				}else
					 goto error;

            }
            free (ports);
        }
        
        if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
			JARLog("cannot find any physical playback ports\n");
         }else{
        
            for (int i = 0; i<TJackClient::fOutputChannels; i++) {
                #if PRINTDEBUG
                    if (ports[i]) JARLog("ports[i] %s\n",ports[i]);
                    if (fOutputPortList[i] && jack_port_name(fOutputPortList[i])) 
                        JARLog("jack_port_name(fOutputPortList[i]) %s\n",jack_port_name(fOutputPortList[i]));
                #endif
                
                // Stop condition
                if (ports[i] == 0) break;
				
				if (fOutputPortList[i] && jack_port_name(fOutputPortList[i])) {
					if (jack_connect(fClient,jack_port_name(fOutputPortList[i]), ports[i])) {
						JARLog("cannot connect ouput ports\n");
					}
				}else
					goto error;
            }
            free (ports);
        }
 	}
	
	RestoreConnections();
    return true;
    
 error:
    JARLog("Jack_Activate error\n");
    return false;
}

//------------------------------------------------------------------------
bool TJackClient::Desactivate()
{
    if (jack_deactivate(fClient)) {
		JARLog("cannot deactivate client");
		return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceAddIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc, void* context)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceAddIOProc called inSelf, proc %ld %x \n", (long)inSelf, proc);
    
    CheckRunning(inSelf);
    TJackClient* client = TJackClient::GetJackClient();
    
    if (client) {
        JARLog("DeviceAddIOProc : add a new proc\n");
        if (client->AddIOProc(proc,context)) IncRefInternal();
        return kAudioHardwareNoError;
    }else{
		JARLog("DeviceAddIOProc : no client \n");
		return kAudioHardwareBadDeviceError;
    }
 }

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceRemoveIOProc(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceRemoveIOProc called inSelf, proc %ld %x \n", (long)inSelf, proc);
 
    CheckRunning(inSelf);
    TJackClient* client = TJackClient::fJackClient; 
    
    JARLog("DeviceRemoveIOProc GetJackClient %x client\n",client);
    
    if (client) {
        if (client->RemoveIOProc(proc)) DecRefInternal();
        return kAudioHardwareNoError;
    }else{
		JARLog("DeviceRemoveIOProc : no client \n");
		return kAudioHardwareBadDeviceError;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceStart(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStart called inSelf, proc %ld %x \n", (long)inSelf, proc);
 
    CheckRunning(inSelf);
    TJackClient* client = TJackClient::fJackClient;
    
    if (client){
        client->Start(proc);
        JARLog("DeviceStart fProcRunning %ld \n", client->fProcRunning);
    }else{
        JARLog("DeviceStart error : null client\n");
        return kAudioHardwareBadDeviceError;
    }
      
    return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceStop(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, AudioDeviceIOProc proc)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStop called inSelf, proc %ld %x \n", (long)inSelf, proc);
 
    CheckRunning(inSelf);
    TJackClient* client = TJackClient::fJackClient;
    
    if (client){
        client->Stop(proc);
        JARLog("DeviceStop fProcRunning %ld \n", client->fProcRunning);
    }else{
        JARLog("DeviceStop error : null client\n");
        return kAudioHardwareBadDeviceError;
    }
 
    return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceRead(AudioHardwarePlugInRef inSelf, AudioDeviceID inDevice, const AudioTimeStamp* inStartTime, AudioBufferList* outData)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceRead : not yet implemented\n");
    return kAudioHardwareUnsupportedOperationError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetCurrentTime(AudioHardwarePlugInRef inSelf,AudioDeviceID inDevice, AudioTimeStamp* outTime)
{
    if (TJackClient::fJackClient){
		outTime->mSampleTime = TJackClient::fJackClient->GetTime();
        outTime->mHostTime = AudioGetCurrentHostTime();
        outTime->mRateScalar = 1.0;
        outTime->mWordClockTime = 0;
        outTime->mFlags = kAudioTimeFlags;
        return kAudioHardwareNoError;
    }else
        return kAudioHardwareNotRunningError;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceTranslateTime(AudioHardwarePlugInRef inSelf,
                                        AudioDeviceID inDevice,
                                        const AudioTimeStamp* inTime,
                                        AudioTimeStamp* outTime)
{
	JARLog("--------------------------------------------------------\n");
	JARLog("DeviceTranslateTime : not yet implemented\n");
	return kAudioHardwareUnsupportedOperationError;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Device Property Management
//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetPropertyInfo(AudioHardwarePlugInRef inSelf, 
                                            AudioDeviceID inDevice, 
                                            UInt32 inChannel, 
                                            Boolean isInput, 
                                            AudioDevicePropertyID inPropertyID, 
                                            UInt32* outSize, 
                                            Boolean* outWritable)
{
	OSStatus err = kAudioHardwareNoError;

	JARLog("--------------------------------------------------------\n");
	JARLog("DeviceGetPropertyInfo inSelf inDevice inChannel isInput  %ld %ld %ld %ld %ld %ld \n",(long)inSelf,inDevice,inChannel,isInput,outSize,outWritable);

	CheckRunning(inSelf);
	Print4CharCode("DeviceGetPropertyInfo ", inPropertyID);

	if (inDevice != TJackClient::fDeviceID)
	{
 		JARLog("DeviceGetPropertyInfo called for invalid device ID\n");
		return kAudioHardwareBadDeviceError;
	}
    
    if (outSize == NULL)
	{
		JARLog("DeviceGetPropertyInfo received NULL outSize pointer\n");
	}
    
    if (outWritable != NULL){
        *outWritable = false;
    }else{
        JARLog("DeviceGetPropertyInfo received NULL outWritable pointer\n");
    }
 	
	switch (inPropertyID)
	{
	#ifdef kAudioHardwarePlugInInterface2ID
			// For applications that needs a output channels map, and for audio midi setup (that will not crash)
		case kAudioDevicePropertyPreferredChannelLayout: 
			if (outSize) *outSize = offsetof(AudioChannelLayout,mChannelDescriptions[TJackClient::fOutputChannels]);
			break;
	#endif

		case kAudioDevicePropertyDeviceName:
			if (outSize) *outSize = TJackClient::fDeviceName.size()+1;
			break;
					
		case kAudioDevicePropertyDeviceManufacturer:
			if (outSize) *outSize = TJackClient::fDeviceManufacturer.size()+1;
			break;
				
		case kAudioDevicePropertyDeviceNameCFString:
		case kAudioDevicePropertyDeviceUID:
			if (outSize) *outSize = sizeof(CFStringRef); // TO BE CHECKED
			break;
		
		case kAudioDevicePropertyPlugIn:
			if (outSize) *outSize = sizeof(OSStatus); 
			break;
				
		case kAudioDevicePropertyTransportType:
		case kAudioDevicePropertyDeviceIsAlive:
		case kAudioDevicePropertyDeviceIsRunning:
		case kAudioDevicePropertyDeviceIsRunningSomewhere:
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
		case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
		case kAudioDeviceProcessorOverload:
			if (outSize) *outSize = sizeof(UInt32); 
			break;
				
		case kAudioDevicePropertyHogMode:
			if (outSize) *outSize = sizeof(pid_t); 
			break;
				
		case kAudioDevicePropertyRegisterBufferList:
			if (outSize) *outSize = 4+sizeof(AudioBuffer)*(TJackClient::fOutputChannels+TJackClient::fInputChannels);
			break;
				
		case kAudioDevicePropertyLatency:
		case kAudioDevicePropertyBufferSize:
		case kAudioDevicePropertyBufferFrameSize:
			if (outSize) *outSize = sizeof(UInt32); 
			break;
			
		case kAudioDevicePropertyUsesVariableBufferFrameSizes:
			err = kAudioHardwareUnknownPropertyError;
			break;
		
		case kAudioDevicePropertyJackIsConnected:
			err = kAudioHardwareUnknownPropertyError;
			break;
				
		case kAudioDevicePropertyBufferSizeRange:
		case kAudioDevicePropertyBufferFrameSizeRange:
			if (outSize) *outSize = sizeof(AudioValueRange); 
			break;
				
		case kAudioDevicePropertyStreams:
			if (outSize){
				if (isInput)
					*outSize = sizeof(AudioStreamID)*TJackClient::fInputChannels;
				else
					*outSize = sizeof(AudioStreamID)*TJackClient::fOutputChannels;
			}
			break;
				
		case kAudioDevicePropertySafetyOffset:
		case kAudioDevicePropertySupportsMixing:
			if (outSize) *outSize = sizeof(UInt32); 
			break;
	
		case kAudioDevicePropertyStreamConfiguration:
			if (outSize){
				if (isInput)
					*outSize = sizeof(UInt32) + sizeof(AudioBuffer)*TJackClient::fInputChannels;
				else
					*outSize =  sizeof(UInt32) + sizeof(AudioBuffer)*TJackClient::fOutputChannels;
				JARLog("DeviceGetPropertyInfo::kAudioDevicePropertyStreamConfiguration %ld\n", *outSize);
			}
			break;
				
		case kAudioDevicePropertyIOProcStreamUsage:
			if (outSize) {
				if (isInput)
					*outSize = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32)*TJackClient::fInputChannels; 
				else
					*outSize = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32)*TJackClient::fOutputChannels; 
				JARLog("DeviceGetPropertyInfo::kAudioDevicePropertyIOProcStreamUsage %ld %ld\n", isInput, *outSize);
			}
			break;
				
		case kAudioDevicePropertyPreferredChannelsForStereo:
			if (outSize) *outSize = sizeof(stereoList);
			break;
				
		case kAudioDevicePropertyNominalSampleRate:
		case kAudioDevicePropertyActualSampleRate:
			if (outSize) *outSize = sizeof(Float64);
			break;

		case kAudioDevicePropertyAvailableNominalSampleRates:
			if (outSize) *outSize = sizeof(AudioValueRange);
			break;
		
		case kAudioDevicePropertyStreamFormat:
		case kAudioStreamPropertyPhysicalFormat:
		case kAudioDevicePropertyStreamFormatSupported:
		case kAudioDevicePropertyStreamFormatMatch:
			if (outSize) *outSize = sizeof(AudioStreamBasicDescription);
			break;   

		case kAudioDevicePropertyStreamFormats:  // TO BE CHECKED
		case kAudioStreamPropertyPhysicalFormats:
			if (outSize) *outSize = sizeof(AudioStreamBasicDescription);
			break; 
		
		case kAudioDevicePropertyDataSource:
		case kAudioDevicePropertyDataSources: 
			//if (outSize) *outSize = sizeof(UInt32);
			err = kAudioHardwareUnknownPropertyError;
			break;
				
		case kAudioDevicePropertyDataSourceNameForID:
		case kAudioDevicePropertyDataSourceNameForIDCFString:
			//if (outSize) *outSize = sizeof(AudioValueTranslation);
			err = kAudioHardwareUnknownPropertyError;
			break;
			   
		// Special Property to access Jack client from application code
		case kAudioDevicePropertyGetJackClient:
			if (outSize) *outSize = sizeof(jack_client_t*); 
			break;
		
		// Redirect call on used CoreAudio driver
		case kAudioDevicePropertyClockSource:
		case kAudioDevicePropertyClockSources:
		case kAudioDevicePropertyClockSourceNameForID:
		case kAudioDevicePropertyClockSourceNameForIDCFString:
		case kAudioHardwarePropertyBootChimeVolumeScalar:
		case kAudioDevicePropertyDriverShouldOwniSub:
		case kAudioDevicePropertyVolumeScalar:
		case kAudioDevicePropertyVolumeDecibels:
		case kAudioDevicePropertyVolumeRangeDecibels:
		case kAudioDevicePropertyVolumeScalarToDecibels:
		case kAudioDevicePropertyVolumeDecibelsToScalar:
		case kAudioDevicePropertyMute:
		case kAudioDevicePropertyPlayThru:
		case kAudioDevicePropertySubVolumeScalar:
		case kAudioDevicePropertySubVolumeDecibels:
		case kAudioDevicePropertySubVolumeRangeDecibels:
		case kAudioDevicePropertySubVolumeScalarToDecibels:
		case kAudioDevicePropertySubVolumeDecibelsToScalar:
		case kAudioDevicePropertySubMute:
	   {
			JARLog("Redirect call on used CoreAudio driver ID %ld \n",TJackClient::fCoreAudioDriver); 
			Print4CharCode("property ",inPropertyID);
			err = AudioDeviceGetPropertyInfo(TJackClient::fCoreAudioDriver, inChannel, isInput, inPropertyID,outSize, outWritable);
			JARLog("Redirect call on used CoreAudio driver err %ld \n",err); 
			printError(err);
			break;
		}
			 
	default:
		Print4CharCode("DeviceGetPropertyInfo unkown request:", inPropertyID);
		err = kAudioHardwareUnknownPropertyError;
		break;
	}
 
	return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        UInt32 inChannel, 
                                        Boolean isInput, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32* ioPropertyDataSize, 
                                        void* outPropertyData)
{
    OSStatus err = kAudioHardwareNoError;
        
    CheckRunning(inSelf);
         
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceGetProperty inSelf isInput inDevice %ld %ld %ld\n",(long)inSelf,isInput,inDevice);
	Print4CharCode("DeviceGetProperty ", inPropertyID);

	if (inDevice != TJackClient::fDeviceID)
	{
		JARLog("DeviceGetProperty called for invalid device ID\n");
 		return kAudioHardwareBadDeviceError;
	}
	
	// steph 14/01/04
	/*
	if (outPropertyData == NULL)
	{
    #if PRINTDEBUG
		JARLog("DeviceGetProperty received NULL pointer\n");
    #endif
		return kAudioHardwareBadPropertySizeError;
	}
	*/
    
	switch (inPropertyID)
	{
    
    #ifdef kAudioHardwarePlugInInterface2ID
        // For applications that needs a output channels map, and for audio midi setup (that will not crash)
        case kAudioDevicePropertyPreferredChannelLayout:                
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioChannelLayout);
			}else if(*ioPropertyDataSize < sizeof(AudioChannelLayout)) {
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError; 
            } else {
                AudioChannelLayout *res = (AudioChannelLayout*)outPropertyData;
                res->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
                res->mChannelBitmap = 0;
                res->mNumberChannelDescriptions = TJackClient::fOutputChannels;
                
                int firstType = 1; //see line 325 of CoreAudioTypes.h (panther)
                
                for(int i = 0;i<TJackClient::fOutputChannels;i++) {
                    res->mChannelDescriptions[i].mChannelLabel = firstType;
                    firstType++; 
                }
                
                *ioPropertyDataSize = sizeof(AudioChannelLayout);
            }
            break;
        }
    #endif
                
		case kAudioDevicePropertyDeviceName:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = TJackClient::fDeviceName.size()+1;
			}else if (*ioPropertyDataSize < TJackClient::fDeviceName.size()+1){
				err = kAudioHardwareBadPropertySizeError;
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
			}else{
				char* data = (char*) outPropertyData;
				strcpy(data,TJackClient::fDeviceName.c_str());
				*ioPropertyDataSize = TJackClient::fDeviceName.size()+1;
			}
			break;
		}
                
		case kAudioDevicePropertyDeviceNameCFString:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(CFStringRef);
			}else if (*ioPropertyDataSize < sizeof (CFStringRef)){
				err = kAudioHardwareBadPropertySizeError;
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
			}else{
				CFStringRef theUIDString = NULL;
				CFStringRef* outString = (CFStringRef*) outPropertyData;
				theUIDString = CFStringCreateWithCString(NULL, TJackClient::fDeviceName.c_str(), CFStringGetSystemEncoding());
				*outString = theUIDString;
				*ioPropertyDataSize = sizeof(CFStringRef);
			}
			break;
		}
                
		case kAudioDevicePropertyDeviceManufacturer:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = TJackClient::fDeviceManufacturer.size()+1;
			}else if (*ioPropertyDataSize < TJackClient::fDeviceManufacturer.size()+1){
				err = kAudioHardwareBadPropertySizeError;
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
			}else{
				char* data = (char*) outPropertyData;
				strcpy(data,TJackClient::fDeviceManufacturer.c_str());
				*ioPropertyDataSize = TJackClient::fDeviceManufacturer.size()+1;
				err = kAudioHardwareNoError;
			}
			break;
		}
                
		case kAudioDevicePropertyDeviceManufacturerCFString:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(CFStringRef);
			}else if (*ioPropertyDataSize < sizeof(CFStringRef)){
				err = kAudioHardwareBadPropertySizeError;
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
			}else{
				CFStringRef theUIDString = NULL;
				CFStringRef* outString = (CFStringRef*) outPropertyData;
				theUIDString = CFStringCreateWithCString(NULL, TJackClient::fDeviceManufacturer.c_str(), CFStringGetSystemEncoding());
				*outString = theUIDString;
				*ioPropertyDataSize = sizeof(CFStringRef);
			}
			break;
		}
                
		case kAudioDevicePropertyDeviceUID:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(CFStringRef);
			}else if (*ioPropertyDataSize < sizeof(CFStringRef)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				CFStringRef theUIDString = NULL;
				CFStringRef* outString = (CFStringRef*) outPropertyData;
				theUIDString = CFStringCreateWithCString(NULL, "JackAudioServer:0", CFStringGetSystemEncoding());
				*outString = theUIDString;
				*ioPropertyDataSize = sizeof(CFStringRef);
			}
			break;
		}
                
		case kAudioDevicePropertyTransportType:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = kIOAudioDeviceTransportTypeBuiltIn;
				*ioPropertyDataSize = sizeof(UInt32);
			}
			break;
        }      
                  
		case kAudioDevicePropertyDeviceIsAlive:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = TJackClient::fDeviceRunning;
				*ioPropertyDataSize = sizeof(UInt32);
			}
			break;
        }
                
  		case kAudioDevicePropertyBufferFrameSize:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = TJackClient::fBufferSize;
				*ioPropertyDataSize = sizeof(UInt32);
			}
			break;
        }
                        
        case kAudioDevicePropertyBufferFrameSizeRange:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}else if (*ioPropertyDataSize < sizeof(AudioValueRange)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				AudioValueRange* range = (AudioValueRange*) outPropertyData;
				range->mMinimum = TJackClient::fBufferSize;
				range->mMaximum = TJackClient::fBufferSize;
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}
			break;        
        }
        
        case kAudioDevicePropertyBufferSize:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = TJackClient::fBufferSize*sizeof(float);
				*ioPropertyDataSize = sizeof(UInt32);
			}
			break;
        }
                
        case kAudioDevicePropertyBufferSizeRange:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}else if (*ioPropertyDataSize < sizeof(AudioValueRange)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				AudioValueRange* range = (AudioValueRange*) outPropertyData;
				range->mMinimum = TJackClient::fBufferSize*sizeof(float);
				range->mMaximum = TJackClient::fBufferSize*sizeof(float);
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}
			break;        
        }
         
        case kAudioDevicePropertyStreamConfiguration:
		{
			long channels = (isInput) ? TJackClient::fInputChannels : TJackClient::fOutputChannels;
			
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32)+sizeof(AudioBuffer)*channels;
			}else if (*ioPropertyDataSize < (sizeof(UInt32)+sizeof(AudioBuffer)*channels)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
			
				AudioBufferList* bList = (AudioBufferList*) outPropertyData;
				bList->mNumberBuffers = channels;
				
				for (int i = 0; i< bList->mNumberBuffers; i++) {
					bList->mBuffers[i].mNumberChannels = 1;
					bList->mBuffers[i].mDataByteSize = TJackClient::fBufferSize*sizeof(float);
					bList->mBuffers[i].mData = NULL;
				}
					
				*ioPropertyDataSize = sizeof(UInt32)+sizeof(AudioBuffer)*channels;
				JARLog("DeviceGetProperty::kAudioDevicePropertyStreamConfiguration %ld\n",  *ioPropertyDataSize);
			}
			break;
		}
                
		case kAudioDevicePropertyStreams:
        {
			long channels = (isInput) ? TJackClient::fInputChannels : TJackClient::fOutputChannels;
			
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioStreamID)*channels;
			}else if (*ioPropertyDataSize < (sizeof(AudioStreamID)*channels)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
			
				AudioStreamID* streamIDList = (AudioStreamID*)outPropertyData;
				
				if (isInput) {
					for (int i = 0; i < channels; i++) {
						streamIDList[i] = TJackClient::fStreamIDList[i];
					}
				}else{
					for (int i = 0; i < channels; i++) {
						streamIDList[i] = TJackClient::fStreamIDList[i+TJackClient::fInputChannels];
					}
				}
					
				*ioPropertyDataSize = sizeof(AudioStreamID)*channels;
			}
			break;
        }
                          
		case kAudioDevicePropertyStreamFormatSupported:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				 err = kAudioHardwareBadPropertySizeError;
			}else{
				JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported \n");
				AudioStreamBasicDescription* desc = (AudioStreamBasicDescription*) outPropertyData;
				bool res = true;
				
				JARLog("Sample Rate:        %f\n", desc->mSampleRate);
				JARLog("Encoding:           %d\n", (int)desc->mFormatID);
				JARLog("FormatFlags:        %d\n", (int)desc->mFormatFlags);
			 
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				
				res &= (desc->mSampleRate != 0) ? (desc->mSampleRate == fSampleRate) : true;
				res &= (desc->mFormatID != 0) ? (desc->mFormatID == kIOAudioStreamSampleFormatLinearPCM) : true;
				res &= (desc->mFormatFlags != 0) ? (desc->mFormatFlags == kJackStreamFormat) : true;
				res &= (desc->mBytesPerPacket != 0) ? (desc->mBytesPerPacket == 4) : true;
				res &= (desc->mFramesPerPacket != 0) ? (desc->mFramesPerPacket == 1) : true;
				res &= (desc->mBytesPerFrame != 0) ? (desc->mBytesPerFrame == 4) : true;
				res &= (desc->mChannelsPerFrame != 0) ? (desc->mChannelsPerFrame == 1) : true;
				res &= (desc->mBitsPerChannel != 0) ? (desc->mBitsPerChannel == 32) : true;
					
				if (res){
					err = kAudioHardwareNoError;
					JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported : format SUPPORTED\n");
				}else{
					JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatSupported : format UNSUPPORTED\n");
					err = kAudioDeviceUnsupportedFormatError;
				}
			}
			break;
        }
                
        case kAudioDevicePropertyStreamFormatMatch:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				JARLog("DeviceGetProperty::kAudioDevicePropertyStreamFormatMatch \n");
				AudioStreamBasicDescription* desc = (AudioStreamBasicDescription*) outPropertyData;
					
				JARLog("Sample Rate:        %f\n", desc->mSampleRate);
				JARLog("Encoding:           %d\n", (int)desc->mFormatID);
				JARLog("FormatFlags:        %d\n", (int)desc->mFormatFlags);
			   
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				
				if (desc->mSampleRate != 0) desc->mSampleRate = fSampleRate;
				if (desc->mFormatID != 0) desc->mFormatID = kIOAudioStreamSampleFormatLinearPCM;
				if (desc->mFormatFlags != 0) desc->mFormatFlags = kJackStreamFormat;
				if (desc->mBytesPerPacket != 0) desc->mBytesPerPacket = 4;
				if (desc->mFramesPerPacket != 0) desc->mFramesPerPacket = 1;
				if (desc->mBytesPerFrame != 0) desc->mBytesPerFrame = 4;
				if (desc->mChannelsPerFrame != 0) desc->mChannelsPerFrame = 1;
				if (desc->mBitsPerChannel != 0) desc->mBitsPerChannel = 32;
			}
			break;
		}

                
        case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
		case kAudioDevicePropertyDeviceCanBeDefaultDevice:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
			   JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = 1;
				*ioPropertyDataSize = sizeof(UInt32);
				JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceCanBeDefaultDevice %ld\n",(long)isInput);
			}
			break;
        }
               
        case kAudioDevicePropertyStreamFormat:
        case kAudioStreamPropertyPhysicalFormat:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				AudioStreamBasicDescription* streamDesc = (AudioStreamBasicDescription*) outPropertyData;
				streamDesc->mSampleRate = fSampleRate;
				streamDesc->mFormatID = kIOAudioStreamSampleFormatLinearPCM;
				streamDesc->mFormatFlags = kJackStreamFormat;
				streamDesc->mBytesPerPacket = 4;
				streamDesc->mFramesPerPacket = 1;
				streamDesc->mBytesPerFrame = 4;
				streamDesc->mChannelsPerFrame = 1;
				streamDesc->mBitsPerChannel = 32;
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}
			break;
		}
                
        case kAudioDevicePropertyStreamFormats:
        case kAudioStreamPropertyPhysicalFormats:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				AudioStreamBasicDescription* streamDescList = (AudioStreamBasicDescription*) outPropertyData;
				streamDescList[0].mSampleRate = fSampleRate;
				streamDescList[0].mFormatID = kIOAudioStreamSampleFormatLinearPCM;
				streamDescList[0].mFormatFlags = kJackStreamFormat;
				streamDescList[0].mBytesPerPacket = 4;
				streamDescList[0].mFramesPerPacket = 1;
				streamDescList[0].mBytesPerFrame = 4;
				streamDescList[0].mChannelsPerFrame = 1;
				streamDescList[0].mBitsPerChannel = 32;
				*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
			}
			break;
		}
                        
		case kAudioDevicePropertyLatency:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = 512; // TO BE IMPLEMENTED
				*ioPropertyDataSize = sizeof(UInt32);
				JARLog("DeviceGetProperty::kAudioDevicePropertyLatency %ld \n",*(UInt32*) outPropertyData);
			}
			break;
		}
        
        case kAudioDevicePropertyHogMode:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(pid_t);
			}else if (*ioPropertyDataSize < sizeof(pid_t)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				// TO BE CHECKED
				pid_t* pid = (pid_t*) outPropertyData;
				*pid = -1;
				//*pid = getpid();
				*ioPropertyDataSize = sizeof(pid_t);
			}
			break;
        }
        
	    case kAudioDevicePropertyNominalSampleRate:
	   {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(Float64);
			}else if (*ioPropertyDataSize < sizeof(Float64)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				Float64* valRange = (Float64*)outPropertyData;
				*valRange = fSampleRate;
				*ioPropertyDataSize = sizeof(Float64);
			}
			break;
		}    
		
        case kAudioDevicePropertyActualSampleRate:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(Float64);
			}else if (*ioPropertyDataSize < sizeof(Float64)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				Float64* valRange = (Float64*)outPropertyData;
				bool running = (TJackClient::fJackClient) ? TJackClient::fJackClient->IsRunning() : false; 
				*valRange = (running) ? fSampleRate : 0.0f;
				*ioPropertyDataSize = sizeof(Float64);
			}
			break;
		}           
        
        case kAudioDevicePropertyAvailableNominalSampleRates:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}else if (*ioPropertyDataSize < sizeof(AudioValueRange)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				AudioValueRange* valRange = (AudioValueRange*)outPropertyData;
				valRange->mMinimum = fSampleRate;
				valRange->mMaximum = fSampleRate;
				*ioPropertyDataSize = sizeof(AudioValueRange);
			}
			break;
        }
        
        case kAudioDevicePropertyDeviceIsRunning: 
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = (TJackClient::fJackClient) ? TJackClient::fJackClient->IsRunning() : false; 
				*ioPropertyDataSize = sizeof(UInt32);
				JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceIsRunning %ld \n", *(UInt32*) outPropertyData);
			}
			break; 
		}
        
        case kAudioDevicePropertyDeviceIsRunningSomewhere:    
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
			}else{
				*(UInt32*) outPropertyData = true; 
				*ioPropertyDataSize = sizeof(UInt32);
				JARLog("DeviceGetProperty::kAudioDevicePropertyDeviceIsRunning %ld \n", *(UInt32*) outPropertyData);
			}
			break; 
		}
                
        case kAudioDevicePropertyPreferredChannelsForStereo:
        {
		     if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(stereoList);
			}else if (*ioPropertyDataSize < sizeof(stereoList)){
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError;
            }else{
                stereoList* list = (stereoList*) outPropertyData;
                list->channel[0] = 1;
                list->channel[1] = 2;
                 *ioPropertyDataSize = sizeof(stereoList);
            }
            break;
        }
            
        case kAudioDevicePropertySafetyOffset:
        {
		     if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError;
            }else{
                *(UInt32*) outPropertyData = 128; // TO BE IMPLEMENTED
                *ioPropertyDataSize = sizeof(UInt32);
            }
            break;
		}
                
        case kAudioDevicePropertyDataSource:
        case kAudioDevicePropertyDataSources:
            /*
            if (*ioPropertyDataSize < sizeof (UInt32)){
            #if PRINTDEBUG
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
            #endif    
                err = kAudioHardwareBadPropertySizeError;
            }else{
                *(UInt32*) outPropertyData = (isInput) ? INPUT_MICROPHONE : OUTPUT_SPEAKER; // TO BE CHECKED
                *ioPropertyDataSize = sizeof (UInt32);
            */
            err = kAudioHardwareUnknownPropertyError;
            break;
                
        case kAudioDevicePropertyPlugIn:
		{
		     if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(OSStatus);
			}else if (*ioPropertyDataSize < sizeof(OSStatus)){
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError;
            }else{
                *(OSStatus*) outPropertyData = kAudioHardwareNoError; // TO BE CHECKED
                *ioPropertyDataSize = sizeof(OSStatus);
            }
            break;
		}
                
        case kAudioDevicePropertyJackIsConnected:
            /*
            if (*ioPropertyDataSize < sizeof (UInt32)){
            #if PRINTDEBUG
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
            #endif    
                err = kAudioHardwareBadPropertySizeError;
            }else{
                //*(UInt32*) outPropertyData = true; // TO BE CHECKED
                *(UInt32*) outPropertyData = false; // TO BE CHECKED
                *ioPropertyDataSize = sizeof (UInt32);
            }
            */
            err = kAudioHardwareUnknownPropertyError;
            break;
                
        // Special Property to access Jack client from application code
        case kAudioDevicePropertyGetJackClient:
        {
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(jack_client_t*);
			}else if (*ioPropertyDataSize < sizeof(jack_client_t*)){
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError;
            }else{
                *ioPropertyDataSize = sizeof(jack_client_t*);
                JARLog("DeviceGetProperty::kAudioDevicePropertyGetJackClient\n");
				if (TJackClient* client = GetJackClient()) {
			         IncRefExternal();
			        *(jack_client_t **) outPropertyData = client->fClient;
			         err = kAudioHardwareNoError;
                }else{
					*(jack_client_t **) outPropertyData = NULL;
                    err = kAudioHardwareBadDeviceError;
                }
            }
            break;
        }
        
        // Special Property to release Jack client from application code
        case kAudioDevicePropertyReleaseJackClient:
        {
            JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
            DecRefExternal(); 
	        break;
        }
        
        case kAudioDevicePropertySupportsMixing:
		{
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = sizeof(UInt32);
			}else if (*ioPropertyDataSize < sizeof(UInt32)){
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				err = kAudioHardwareBadPropertySizeError;
            }else{
                *(UInt32*) outPropertyData = 1; // support Mixing
                *ioPropertyDataSize = sizeof(UInt32);
            }
            break;
		}
                
        case kAudioDevicePropertyUsesVariableBufferFrameSizes: 
            err = kAudioHardwareUnknownPropertyError;
            break;
                
        case kAudioDevicePropertyDataSourceNameForID:
        {
            /*
            if (*ioPropertyDataSize < sizeof (AudioValueTranslation)){
            #if PRINTDEBUG
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
            #endif    
                err = kAudioHardwareBadPropertySizeError;
            }else{
                AudioValueTranslation* translation = (AudioValueTranslation*)outPropertyData;
                char* data = (char*) translation->mOutputData;
                strcpy(data,TJackClient::fDeviceName.c_str());
                translation->mOutputDataSize = TJackClient::fDeviceName.size()+1;
                *ioPropertyDataSize = sizeof(AudioValueTranslation);
            */
            err = kAudioHardwareUnknownPropertyError;
            break;
        }

        case kAudioDevicePropertyDataSourceNameForIDCFString:
        {
            /*
            if (*ioPropertyDataSize < sizeof (AudioValueTranslation)){
            #if PRINTDEBUG
                JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
            #endif    
                err = kAudioHardwareBadPropertySizeError;
            }else{
                CFStringRef theUIDString = NULL;
                AudioValueTranslation* translation = (AudioValueTranslation*)outPropertyData;
                CFStringRef* outString = (CFStringRef*) translation->mOutputData;
                theUIDString = CFStringCreateWithCString(NULL, TJackClient::fDeviceName.c_str(), CFStringGetSystemEncoding());
                *outString = theUIDString;
                translation->mOutputDataSize = sizeof (CFStringRef);
                *ioPropertyDataSize = sizeof(AudioValueTranslation);
            }
            */
            err = kAudioHardwareUnknownPropertyError;
            break;
        }
		
		case kAudioDevicePropertyIOProcStreamUsage:
		{
			long size;
			if (isInput)
				size = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32)*TJackClient::fInputChannels; 
			else
				size = sizeof(void*) + sizeof(UInt32) + sizeof(UInt32)*TJackClient::fOutputChannels; 
						
			if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
				*ioPropertyDataSize = size;
			}else if(*ioPropertyDataSize < size) {
				JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
                err = kAudioHardwareBadPropertySizeError; 
			}else{
				JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage %ld \n",*ioPropertyDataSize);
				AudioHardwareIOProcStreamUsage* outData  = (AudioHardwareIOProcStreamUsage*)outPropertyData;
				JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage mIOProc %x \n", (AudioDeviceIOProc)outData->mIOProc);
      		
				if (isInput) {
					outData->mNumberStreams = TJackClient::fInputChannels;
					for (int i = 0; i<TJackClient::fInputChannels; i++) {
						JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage mStreamIsOn %ld \n",outData->mStreamIsOn[i]);
						outData->mStreamIsOn[i] = 1;
					}
				}else{
					outData->mNumberStreams = TJackClient::fOutputChannels;
					for (int i = 0; i<TJackClient::fOutputChannels; i++) {
						JARLog("DeviceGetProperty : kAudioDevicePropertyIOProcStreamUsage mStreamIsOn %ld \n",outData->mStreamIsOn[i]);
						outData->mStreamIsOn[i] = 1;
					}
				}
				*ioPropertyDataSize = size;
			}
			break;
		}
	          
  
        // Redirect call on used CoreAudio driver
        case kAudioDevicePropertyClockSource:
        case kAudioDevicePropertyClockSources:
        case kAudioDevicePropertyClockSourceNameForID:
        case kAudioDevicePropertyClockSourceNameForIDCFString:
        case kAudioHardwarePropertyBootChimeVolumeScalar:
		case kAudioDevicePropertyDriverShouldOwniSub:
        case kAudioDevicePropertyVolumeScalar:
        case kAudioDevicePropertyVolumeDecibels:
        case kAudioDevicePropertyVolumeRangeDecibels:
        case kAudioDevicePropertyVolumeScalarToDecibels:
        case kAudioDevicePropertyVolumeDecibelsToScalar:
        case kAudioDevicePropertyMute:
        case kAudioDevicePropertyPlayThru:
        case kAudioDevicePropertySubVolumeScalar:
        case kAudioDevicePropertySubVolumeDecibels:
        case kAudioDevicePropertySubVolumeRangeDecibels:
        case kAudioDevicePropertySubVolumeScalarToDecibels:
        case kAudioDevicePropertySubVolumeDecibelsToScalar:
        case kAudioDevicePropertySubMute:
        {
            JARLog("Redirect call on used CoreAudio driver ID %ld \n",TJackClient::fCoreAudioDriver); 
            Print4CharCode("property ",inPropertyID);
			err = AudioDeviceGetProperty(TJackClient::fCoreAudioDriver, 
                                        inChannel, 
                                        isInput, 
                                        inPropertyID,
                                        ioPropertyDataSize, 
                                        outPropertyData);
            
            JARLog("Redirect call on used CoreAudio driver err %ld \n",err); 
            printError(err);
			break;
        }
                           
		default:
		Print4CharCode("DeviceGetProperty unkown request:", inPropertyID);
		err = kAudioHardwareUnknownPropertyError;
		break;
	}
	return err;
}

/*
Some properties like kAudioDevicePropertyBufferSize, kAudioDevicePropertyNominalSampleRate can be "set", even is there is no Jack client running


*/

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceSetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        const AudioTimeStamp* inWhen, 
                                        UInt32 inChannel, 
                                        Boolean isInput, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32 inPropertyDataSize, 
                                        const void* inPropertyData)
{

        JARLog("--------------------------------------------------------\n");
        JARLog("DeviceSetProperty inSelf %ld\n",(long)inSelf);
        CheckRunning(inSelf);

		Print4CharCode ("DeviceSetProperty ",inPropertyID);
        OSStatus err = kAudioHardwareNoError;
        
        if (inDevice != TJackClient::fDeviceID){
            JARLog("DeviceSetProperty called for invalid device ID\n");
            return kAudioHardwareBadDeviceError;
        }
		
		switch (inPropertyID)
        {
        
		#ifdef kAudioHardwarePlugInInterface2ID
			case kAudioDevicePropertyPreferredChannelLayout:
				JARLog("kAudioDevicePropertyPreferredChannelLayout\n");
				err = kAudioHardwareNoError;
				break;
		#endif
				 
			case kAudioDevicePropertyBufferSize:
				JARLog("kAudioDevicePropertyBufferSize %ld \n", *(UInt32*) inPropertyData);
				err = (*(UInt32*) inPropertyData == TJackClient::fBufferSize*sizeof(float)) ? kAudioHardwareNoError
					: kAudioHardwareUnknownPropertyError;
			   break;
				
			case kAudioDevicePropertyBufferFrameSize:
				JARLog("kAudioDevicePropertyBufferFrameSize %ld \n", *(UInt32*) inPropertyData);
				err = (*(UInt32*) inPropertyData == TJackClient::fBufferSize) ? kAudioHardwareNoError
						: kAudioHardwareUnknownPropertyError;
				break;
				
			case kAudioDevicePropertyNominalSampleRate:
			{
				Float64* value = (Float64*)inPropertyData;
				JARLog("kAudioDevicePropertyNominalSampleRate %f \n", *value);
				err =  (*value == TJackClient::fSampleRate) ? kAudioHardwareNoError 
					: kAudioHardwareUnknownPropertyError;
				JARLog("kAudioDevicePropertyNominalSampleRate err %ld \n", err);
				break;
			}
			
		   case  kAudioDevicePropertyStreamFormat:
		   {
				AudioStreamBasicDescription* streamDesc = (AudioStreamBasicDescription*) inPropertyData;
				JARLog("Sample Rate:        %f\n", streamDesc->mSampleRate);
				JARLog("Encoding:           %d\n", (int)streamDesc->mFormatID);
				JARLog("FormatFlags:        %d\n", (int)streamDesc->mFormatFlags);
				JARLog("Bytes per Packet:   %d\n", (int)streamDesc->mBytesPerPacket);
				JARLog("Frames per Packet:  %d\n", (int)streamDesc->mFramesPerPacket);
				JARLog("Bytes per Frame:    %d\n", (int)streamDesc->mBytesPerFrame);
				JARLog("Channels per Frame: %d\n", (int)streamDesc->mChannelsPerFrame);
				JARLog("Bits per Channel:   %d\n", (int)streamDesc->mBitsPerChannel);
				err = kAudioHardwareUnknownPropertyError;
				break;
			}
				  
			case kAudioDevicePropertyDeviceCanBeDefaultDevice:
			case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
				break;
				 
			case kAudioDevicePropertyPlugIn:						
			case kAudioDevicePropertyDeviceUID:						
			case kAudioDevicePropertyTransportType:					
			case kAudioDevicePropertyDeviceIsAlive:					
			case kAudioDevicePropertyDeviceIsRunning:					
			case kAudioDevicePropertyDeviceIsRunningSomewhere:		
			case kAudioDevicePropertyJackIsConnected:					
			case kAudioDeviceProcessorOverload:						
			case kAudioDevicePropertyHogMode:							
			case kAudioDevicePropertyRegisterBufferList:				
			case kAudioDevicePropertyLatency:							
			case kAudioDevicePropertyBufferSizeRange:					
			case kAudioDevicePropertyBufferFrameSizeRange:			
			case kAudioDevicePropertyUsesVariableBufferFrameSizes:	
			case kAudioDevicePropertyStreams:							
			case kAudioDevicePropertySafetyOffset:					
			case kAudioDevicePropertySupportsMixing:					
			case kAudioDevicePropertyStreamConfiguration:
				break;
					
			case kAudioDevicePropertyIOProcStreamUsage: 
			{
				// Can only be set when a client is running
				if (TJackClient::fJackClient == NULL) {
					JARLog("DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage : called when then Jack server is not running\n");
					err = kAudioHardwareBadDeviceError;
				}else{

					AudioHardwareIOProcStreamUsage* inData  = (AudioHardwareIOProcStreamUsage*)inPropertyData;	
					
					JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage size %ld\n",inPropertyDataSize);
					JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage proc %x\n",(AudioDeviceIOProc)inData->mIOProc);
					JARLog("DeviceSetProperty : kAudioDevicePropertyIOProcStreamUsage mNumberStreams %ld\n",inData->mNumberStreams);
				
					map<AudioDeviceIOProc,TProcContext>::iterator iter = TJackClient::fJackClient->fAudioIOProcList.find((AudioDeviceIOProc)inData->mIOProc);
					
					if (iter == TJackClient::fJackClient->fAudioIOProcList.end()) {
						JARLog("DeviceSetProperty  kAudioDevicePropertyIOProcStreamUsage : Proc not found %x \n",(AudioDeviceIOProc)inData->mIOProc);
						err = kAudioHardwareUnknownPropertyError;
					}else {
						JARLog("DeviceSetProperty kAudioDevicePropertyIOProcStreamUsage : Proc found %x \n",(AudioDeviceIOProc)inData->mIOProc);
				
						iter->second.fStreamUsage = true; // We need to take care of stream usage in Process
				
						if (isInput) {
							for (int i = 0; i<inData->mNumberStreams; i++) {
								iter->second.fInput[i] = inData->mStreamIsOn[i];
								JARLog("DeviceSetProperty : input kAudioDevicePropertyIOProcStreamUsage inData->mStreamIsOn %ld \n",inData->mStreamIsOn[i]);
							}
						}else{
							for (int i = 0; i<inData->mNumberStreams; i++) {
								iter->second.fOutput[i] = inData->mStreamIsOn[i];
								JARLog("DeviceSetProperty : output kAudioDevicePropertyIOProcStreamUsage inData->mStreamIsOn %ld \n",inData->mStreamIsOn[i]);
							}   
						}
						err = kAudioHardwareNoError;
					}
				}	
				break;
			}
																					
			case kAudioDevicePropertyPreferredChannelsForStereo:		
			case kAudioDevicePropertyAvailableNominalSampleRates:		
			case kAudioDevicePropertyActualSampleRate:				
			case kAudioDevicePropertyStreamFormats:					
			case kAudioDevicePropertyStreamFormatSupported:			
			case kAudioDevicePropertyStreamFormatMatch:
			case kAudioDevicePropertyDataSource:						
			case kAudioDevicePropertyDataSources:						
			case kAudioDevicePropertyDataSourceNameForID:				
			case kAudioDevicePropertyDataSourceNameForIDCFString:		
			case kAudioDevicePropertyClockSource:						
			case kAudioDevicePropertyClockSources:					
			case kAudioDevicePropertyClockSourceNameForID:			
			case kAudioDevicePropertyClockSourceNameForIDCFString:	
				err = kAudioHardwareUnknownPropertyError;
				break;              
					
			case kAudioDevicePropertyVolumeScalar:					
			case kAudioDevicePropertyVolumeDecibels:				
			case kAudioDevicePropertyVolumeRangeDecibels:				
			case kAudioDevicePropertyVolumeScalarToDecibels:			
			case kAudioDevicePropertyVolumeDecibelsToScalar:
			case kAudioDevicePropertyMute:							
			case kAudioDevicePropertyPlayThru:	
			case kAudioDevicePropertyDriverShouldOwniSub:	
			case kAudioDevicePropertySubVolumeScalar:					
			case kAudioDevicePropertySubVolumeDecibels:				
			case kAudioDevicePropertySubVolumeRangeDecibels:		
			case kAudioDevicePropertySubVolumeScalarToDecibels:		
			case kAudioDevicePropertySubVolumeDecibelsToScalar:		
			case kAudioDevicePropertySubMute:							
			{
				JARLog("Redirect call on used CoreAudio driver ID %ld \n",TJackClient::fCoreAudioDriver); 
				Print4CharCode("property ",inPropertyID);
				err = AudioDeviceSetProperty(TJackClient::fCoreAudioDriver, 
											inWhen,
											inChannel, 
											isInput, 
											inPropertyID,
											inPropertyDataSize, 
											inPropertyData);
				
				JARLog("Redirect call on used CoreAudio driver err %ld \n",err); 
				printError(err);
				break;
			}
	  
			default:
				Print4CharCode("DeviceSetProperty unkown request:", inPropertyID);
				err = kAudioHardwareUnknownPropertyError;
				break;
                
	}
	
	return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Stream Property Management
//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamGetPropertyInfo(AudioHardwarePlugInRef inSelf, 
                                            AudioStreamID inStream, 
                                            UInt32 inChannel, 
                                            AudioDevicePropertyID inPropertyID, 
                                            UInt32* outSize, 
                                            Boolean* outWritable)
{
        OSStatus err = kAudioHardwareNoError;
        JARLog("--------------------------------------------------------\n");
        JARLog("StreamGetPropertyInfo inSelf inPropertyID %ld \n",(long)inSelf);
        CheckRunning(inSelf);
        Print4CharCode("StreamGetPropertyInfo ", inPropertyID);
	
        if(outSize == NULL)
        {
			JARLog("StreamGetPropertyInfo received NULL outSize pointer\n");
        }
        
        if (outWritable != NULL){
            *outWritable = false;
        }else{
            JARLog("StreamGetPropertyInfo received NULL outWritable pointer\n");
        }
    
        switch (inPropertyID)
        {
			case kAudioStreamPropertyOwningDevice:
				if (outSize) *outSize = sizeof(AudioDeviceID);
				break;
					
			case kAudioStreamPropertyDirection:
			case kAudioStreamPropertyTerminalType:
			case kAudioStreamPropertyStartingChannel:
				if (outSize) *outSize = sizeof(UInt32);
				break;
	
			case kAudioDevicePropertyStreamFormat:	
			case kAudioDevicePropertyStreamFormats:				
			case kAudioStreamPropertyPhysicalFormat:
			case kAudioStreamPropertyPhysicalFormats:
			case kAudioStreamPropertyPhysicalFormatSupported:
			case kAudioStreamPropertyPhysicalFormatMatch:
				if (outSize) *outSize = sizeof(AudioStreamBasicDescription);
				break;
			 
			case kAudioDevicePropertyDataSource:
			case kAudioDevicePropertyDataSources: 
				err = kAudioHardwareUnknownPropertyError;
				break;
				
			case kAudioDevicePropertyJackIsConnected:
				err = kAudioHardwareUnknownPropertyError;
				break;

			case kAudioDevicePropertyDeviceName:						
			case kAudioDevicePropertyDeviceNameCFString:				
			case kAudioDevicePropertyDeviceManufacturer:				
			case kAudioDevicePropertyDeviceManufacturerCFString:		
			case kAudioDevicePropertyPlugIn:						
			case kAudioDevicePropertyDeviceUID:						
			case kAudioDevicePropertyTransportType:					
			case kAudioDevicePropertyDeviceIsAlive:					
			case kAudioDevicePropertyDeviceIsRunning:					
			case kAudioDevicePropertyDeviceIsRunningSomewhere:		
			case kAudioDevicePropertyDeviceCanBeDefaultDevice:		
			case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:	
			case kAudioDeviceProcessorOverload:						
			case kAudioDevicePropertyHogMode:							
			case kAudioDevicePropertyRegisterBufferList:				
			case kAudioDevicePropertyLatency:							
			case kAudioDevicePropertyBufferSize:						
			case kAudioDevicePropertyBufferSizeRange:					
			case kAudioDevicePropertyBufferFrameSize:					
			case kAudioDevicePropertyBufferFrameSizeRange:			
			case kAudioDevicePropertyUsesVariableBufferFrameSizes:	
			case kAudioDevicePropertyStreams:							
			case kAudioDevicePropertySafetyOffset:					
			case kAudioDevicePropertySupportsMixing:					
			case kAudioDevicePropertyStreamConfiguration:				
			case kAudioDevicePropertyIOProcStreamUsage:				
			case kAudioDevicePropertyPreferredChannelsForStereo:		
			case kAudioDevicePropertyNominalSampleRate:				
			case kAudioDevicePropertyAvailableNominalSampleRates:		
			case kAudioDevicePropertyActualSampleRate:				
			case kAudioDevicePropertyStreamFormatSupported:			
			case kAudioDevicePropertyStreamFormatMatch:				
			case kAudioDevicePropertyVolumeScalar:					
			case kAudioDevicePropertyVolumeDecibels:				
			case kAudioDevicePropertyVolumeRangeDecibels:				
			case kAudioDevicePropertyVolumeScalarToDecibels:			
			case kAudioDevicePropertyVolumeDecibelsToScalar:			
			case kAudioDevicePropertyMute:							
			case kAudioDevicePropertyPlayThru:						
			case kAudioDevicePropertyDataSourceNameForID:				
			case kAudioDevicePropertyDataSourceNameForIDCFString:		
			case kAudioDevicePropertyClockSource:						
			case kAudioDevicePropertyClockSources:					
			case kAudioDevicePropertyClockSourceNameForID:			
			case kAudioDevicePropertyClockSourceNameForIDCFString:	
			case kAudioDevicePropertyDriverShouldOwniSub:				
			case kAudioDevicePropertySubVolumeScalar:					
			case kAudioDevicePropertySubVolumeDecibels:				
			case kAudioDevicePropertySubVolumeRangeDecibels:		
			case kAudioDevicePropertySubVolumeScalarToDecibels:		
			case kAudioDevicePropertySubVolumeDecibelsToScalar:		
			case kAudioDevicePropertySubMute:					
			{
				JARLog("Error : StreamGetPropertyInfo called for a stream\n"); 
				Print4CharCode("property ",inPropertyID);
				err = kAudioHardwareUnknownPropertyError;
				break;
			}
				   
			default:
				Print4CharCode("StreamGetPropertyInfo unkown request:", inPropertyID);
				err = kAudioHardwareUnknownPropertyError;
				break;
                
	}
	return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamGetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioStreamID inStream, 
                                        UInt32 inChannel, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32* ioPropertyDataSize, 
                                        void* outPropertyData)
{
        OSStatus err = kAudioHardwareNoError;
        JARLog("--------------------------------------------------------\n");
        JARLog("StreamGetProperty inSelf %ld\n",(long)inSelf);
        CheckRunning(inSelf);
		Print4CharCode("StreamGetProperty ", inPropertyID);
              
        switch (inPropertyID)
        {
			case kAudioStreamPropertyDirection:
			{
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(UInt32);
				}else if (*ioPropertyDataSize < sizeof(UInt32)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					for (int i = 0; i<TJackClient::fInputChannels; i++) {
						if (inStream == fStreamIDList[i]){
							*(UInt32*) outPropertyData = 1;
							JARLog("StreamGetProperty FOUND INPUT %ld\n", inStream);
							break;
						}
					} 
					
					for (int i = TJackClient::fInputChannels; i < (TJackClient::fOutputChannels + TJackClient::fInputChannels); i++) {
						if (inStream == fStreamIDList[i]){
							*(UInt32*) outPropertyData = 0;
							JARLog("StreamGetProperty FOUND OUTPUT %ld\n", inStream);
							break;
						}
					} 
					*ioPropertyDataSize = sizeof(UInt32);
				}
				break;
			}
                
			case kAudioStreamPropertyStartingChannel:
			{
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(UInt32);
				}else if (*ioPropertyDataSize < sizeof(UInt32)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					*(UInt32*) outPropertyData = 1; // TO BE CHECKED
					*ioPropertyDataSize = sizeof(UInt32);
				}
				break;
			}
                
          	case kAudioStreamPropertyTerminalType:
			{
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(UInt32);
				}else if (*ioPropertyDataSize < sizeof(UInt32)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					for (int i = 0; i<TJackClient::fInputChannels; i++) {
						if (inStream == fStreamIDList[i]){
							*(UInt32*) outPropertyData = INPUT_MICROPHONE;
							JARLog("StreamGetProperty FOUND INPUT %ld\n", inStream);
							break;
						}
					} 
					
					for (int i = TJackClient::fInputChannels; i < (TJackClient::fOutputChannels + TJackClient::fInputChannels); i++) {
						if (inStream == fStreamIDList[i]){
							*(UInt32*) outPropertyData = OUTPUT_SPEAKER;
							JARLog("StreamGetProperty FOUND OUTPUT %ld\n", inStream);
							break;
						}
					} 
					*ioPropertyDataSize = sizeof(UInt32);
				}
				break;
			}
                      
            case kAudioDevicePropertyStreamFormat:
         	case kAudioStreamPropertyPhysicalFormat:
         	{
                
			#if PRINTDEBUG
				if (inPropertyID == kAudioDevicePropertyStreamFormat){
					JARLog("StreamGetProperty : GET kAudioDevicePropertyStreamFormat %ld\n",*ioPropertyDataSize);
				}else{
					JARLog("StreamGetProperty : GET kAudioStreamPropertyPhysicalFormat %ld\n",*ioPropertyDataSize); 
				}
			#endif 
			
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					AudioStreamBasicDescription* streamDesc = (AudioStreamBasicDescription*) outPropertyData;
					streamDesc->mSampleRate = fSampleRate;
					streamDesc->mFormatID = kIOAudioStreamSampleFormatLinearPCM;
					streamDesc->mFormatFlags = kJackStreamFormat;
					streamDesc->mBytesPerPacket = 4;
					streamDesc->mFramesPerPacket = 1;
					streamDesc->mBytesPerFrame = 4;
					streamDesc->mChannelsPerFrame = 1;
					streamDesc->mBitsPerChannel = 32;
					*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				}
				break;
            }
                
            case kAudioDevicePropertyStreamFormats:
            case kAudioStreamPropertyPhysicalFormats:
            {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				}else if (*ioPropertyDataSize < sizeof(AudioStreamBasicDescription)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					AudioStreamBasicDescription* streamDescList = (AudioStreamBasicDescription*) outPropertyData;
					streamDescList[0].mSampleRate = fSampleRate;
					streamDescList[0].mFormatID = kIOAudioStreamSampleFormatLinearPCM;
					streamDescList[0].mFormatFlags = kJackStreamFormat;
					streamDescList[0].mBytesPerPacket = 4;
					streamDescList[0].mFramesPerPacket = 1;
					streamDescList[0].mBytesPerFrame = 4;
					streamDescList[0].mChannelsPerFrame = 1;
					streamDescList[0].mBitsPerChannel = 32;
					*ioPropertyDataSize = sizeof(AudioStreamBasicDescription);
				}
				break;
			}
        
            case kAudioStreamPropertyOwningDevice:
            {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = sizeof(AudioDeviceID);
				}else if (*ioPropertyDataSize < sizeof(AudioDeviceID)){
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
					err = kAudioHardwareBadPropertySizeError;
				}else{
					err = kAudioHardwareUnknownPropertyError;
					*ioPropertyDataSize = sizeof(AudioDeviceID);
					for (int i = 0; i<TJackClient::fOutputChannels + TJackClient::fInputChannels; i++) {
						if (fStreamIDList[i] == inStream) {
							*(AudioDeviceID*)outPropertyData = TJackClient::fDeviceID;
							err = kAudioHardwareNoError;
							break;
						}
					}
				}
			}    
            break;
               
            case kAudioStreamPropertyPhysicalFormatSupported:
            case kAudioStreamPropertyPhysicalFormatMatch:
				err = kAudioHardwareUnknownPropertyError;
				break;
                
            case kAudioDevicePropertyDeviceName:
            {
				if ((outPropertyData == NULL) && (ioPropertyDataSize != NULL)){
					*ioPropertyDataSize = TJackClient::fStreamName.size()+1;
				}else if (*ioPropertyDataSize < TJackClient::fStreamName.size()+1){
					err = kAudioHardwareBadPropertySizeError;
					JARLog("StreamGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				}else{
					char* data = (char*) outPropertyData;
					strcpy(data,TJackClient::fStreamName.c_str());
					*ioPropertyDataSize = TJackClient::fStreamName.size()+1;
				}
				break;
            }
            break;
                
            case kAudioDevicePropertyDataSource:
            case kAudioDevicePropertyDataSources:
				/*
				if (*ioPropertyDataSize < sizeof (UInt32)){
				#if PRINTDEBUG
					JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				#endif    
					err = kAudioHardwareBadPropertySizeError;
				}else{
					*(UInt32*) outPropertyData = (isInput) ? INPUT_MICROPHONE : OUTPUT_SPEAKER; // TO BE CHECKED
					*ioPropertyDataSize = sizeof (UInt32);
				}
				*/
				err = kAudioHardwareUnknownPropertyError;
				break;
                
			case kAudioDevicePropertyJackIsConnected:
				/*
				if (*ioPropertyDataSize < sizeof (UInt32)){
				#if PRINTDEBUG
					JARLog("DeviceGetProperty : kAudioHardwareBadPropertySizeError %ld\n",*ioPropertyDataSize);
				#endif    
					err = kAudioHardwareBadPropertySizeError;
				}else{
					//*(UInt32*) outPropertyData = true; // TO BE CHECKED
					*(UInt32*) outPropertyData = false; // TO BE CHECKED
					*ioPropertyDataSize = sizeof (UInt32);
				}
				*/
				err = kAudioHardwareUnknownPropertyError;
				break;
			
			case kAudioDevicePropertyDeviceNameCFString:
			case kAudioDevicePropertyDeviceManufacturer:
			case kAudioDevicePropertyDeviceManufacturerCFString:
			case kAudioDevicePropertyPlugIn:						
			case kAudioDevicePropertyDeviceUID:						
			case kAudioDevicePropertyTransportType:					
			case kAudioDevicePropertyDeviceIsAlive:					
			case kAudioDevicePropertyDeviceIsRunning:					
			case kAudioDevicePropertyDeviceIsRunningSomewhere:		
			case kAudioDevicePropertyDeviceCanBeDefaultDevice:		
			case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:	
			case kAudioDeviceProcessorOverload:						
			case kAudioDevicePropertyHogMode:							
			case kAudioDevicePropertyRegisterBufferList:				
			case kAudioDevicePropertyLatency:							
			case kAudioDevicePropertyBufferSize:						
			case kAudioDevicePropertyBufferSizeRange:					
			case kAudioDevicePropertyBufferFrameSize:					
			case kAudioDevicePropertyBufferFrameSizeRange:			
			case kAudioDevicePropertyUsesVariableBufferFrameSizes:	
			case kAudioDevicePropertyStreams:							
			case kAudioDevicePropertySafetyOffset:					
			case kAudioDevicePropertySupportsMixing:					
			case kAudioDevicePropertyStreamConfiguration:				
			case kAudioDevicePropertyIOProcStreamUsage:				
			case kAudioDevicePropertyPreferredChannelsForStereo:		
			case kAudioDevicePropertyNominalSampleRate:				
			case kAudioDevicePropertyAvailableNominalSampleRates:		
			case kAudioDevicePropertyActualSampleRate:				
			case kAudioDevicePropertyStreamFormatSupported:			
			case kAudioDevicePropertyStreamFormatMatch:				
			case kAudioDevicePropertyVolumeScalar:					
			case kAudioDevicePropertyVolumeDecibels:				
			case kAudioDevicePropertyVolumeRangeDecibels:				
			case kAudioDevicePropertyVolumeScalarToDecibels:			
			case kAudioDevicePropertyVolumeDecibelsToScalar:			
			case kAudioDevicePropertyMute:							
			case kAudioDevicePropertyPlayThru:			
							
			case kAudioDevicePropertyDataSourceNameForID:				
			case kAudioDevicePropertyDataSourceNameForIDCFString:		
			case kAudioDevicePropertyClockSource:						
			case kAudioDevicePropertyClockSources:					
			case kAudioDevicePropertyClockSourceNameForID:			
			case kAudioDevicePropertyClockSourceNameForIDCFString:	
			case kAudioDevicePropertyDriverShouldOwniSub:				
			case kAudioDevicePropertySubVolumeScalar:					
			case kAudioDevicePropertySubVolumeDecibels:				
			case kAudioDevicePropertySubVolumeRangeDecibels:		
			case kAudioDevicePropertySubVolumeScalarToDecibels:		
			case kAudioDevicePropertySubVolumeDecibelsToScalar:		
			case kAudioDevicePropertySubMute:					
				JARLog("Error : StreamGetProperty called for a stream %ld \n",inPropertyID); 
				err = kAudioHardwareUnknownPropertyError;
				break;
	  
			default:
				Print4CharCode("StreamGetProperty unkown request:", inPropertyID);
				err = kAudioHardwareUnknownPropertyError;
				break;
                
	}
	return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::StreamSetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioStreamID inStream, 
                                        const AudioTimeStamp* inWhen, 
                                        UInt32 inChannel, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32 inPropertyDataSize, 
                                        const void* inPropertyData)
{
        OSStatus err = kAudioHardwareNoError;
        JARLog("--------------------------------------------------------\n");
        JARLog("StreamSetProperty inSelf %ld\n",(long)inSelf);
        CheckRunning(inSelf);
        JARLog("StreamSetProperty\n");
        Print4CharCode("StreamSetProperty request:", inPropertyID);
         
        switch(inPropertyID) {
        
            case kAudioStreamPropertyOwningDevice:
            case kAudioStreamPropertyDirection:
            case kAudioStreamPropertyTerminalType:
            case kAudioStreamPropertyStartingChannel:
            case kAudioStreamPropertyPhysicalFormat:
            case kAudioStreamPropertyPhysicalFormats:
            case kAudioStreamPropertyPhysicalFormatSupported:
            case kAudioStreamPropertyPhysicalFormatMatch:
                err = kAudioHardwareUnknownPropertyError;
                break;
            
            case kAudioDevicePropertyDeviceName:						
            case kAudioDevicePropertyDeviceNameCFString:				
            case kAudioDevicePropertyDeviceManufacturer:				
            case kAudioDevicePropertyDeviceManufacturerCFString:		
            case kAudioDevicePropertyPlugIn:						
            case kAudioDevicePropertyDeviceUID:						
            case kAudioDevicePropertyTransportType:					
            case kAudioDevicePropertyDeviceIsAlive:					
            case kAudioDevicePropertyDeviceIsRunning:					
            case kAudioDevicePropertyDeviceIsRunningSomewhere:		
            case kAudioDevicePropertyDeviceCanBeDefaultDevice:		
            case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:	
            case kAudioDevicePropertyJackIsConnected:					
            case kAudioDeviceProcessorOverload:						
            case kAudioDevicePropertyHogMode:							
            case kAudioDevicePropertyRegisterBufferList:				
            case kAudioDevicePropertyLatency:							
            case kAudioDevicePropertyBufferSize:						
            case kAudioDevicePropertyBufferSizeRange:					
            case kAudioDevicePropertyBufferFrameSize:					
            case kAudioDevicePropertyBufferFrameSizeRange:			
            case kAudioDevicePropertyUsesVariableBufferFrameSizes:	
            case kAudioDevicePropertyStreams:							
            case kAudioDevicePropertySafetyOffset:					
            case kAudioDevicePropertySupportsMixing:					
            case kAudioDevicePropertyStreamConfiguration:				
            case kAudioDevicePropertyIOProcStreamUsage:				
            case kAudioDevicePropertyPreferredChannelsForStereo:		
            case kAudioDevicePropertyNominalSampleRate:				
            case kAudioDevicePropertyAvailableNominalSampleRates:		
            case kAudioDevicePropertyActualSampleRate:				
            case kAudioDevicePropertyStreamFormat:					
            case kAudioDevicePropertyStreamFormats:					
            case kAudioDevicePropertyStreamFormatSupported:			
            case kAudioDevicePropertyStreamFormatMatch:				
            case kAudioDevicePropertyVolumeScalar:					
            case kAudioDevicePropertyVolumeDecibels:				
            case kAudioDevicePropertyVolumeRangeDecibels:				
            case kAudioDevicePropertyVolumeScalarToDecibels:			
            case kAudioDevicePropertyVolumeDecibelsToScalar:			
            case kAudioDevicePropertyMute:							
            case kAudioDevicePropertyPlayThru:						
            case kAudioDevicePropertyDataSource:						
            case kAudioDevicePropertyDataSources:						
            case kAudioDevicePropertyDataSourceNameForID:				
            case kAudioDevicePropertyDataSourceNameForIDCFString:		
            case kAudioDevicePropertyClockSource:						
            case kAudioDevicePropertyClockSources:					
            case kAudioDevicePropertyClockSourceNameForID:			
            case kAudioDevicePropertyClockSourceNameForIDCFString:	
            case kAudioDevicePropertyDriverShouldOwniSub:				
            case kAudioDevicePropertySubVolumeScalar:					
            case kAudioDevicePropertySubVolumeDecibels:				
            case kAudioDevicePropertySubVolumeRangeDecibels:		
            case kAudioDevicePropertySubVolumeScalarToDecibels:		
            case kAudioDevicePropertySubVolumeDecibelsToScalar:		
            case kAudioDevicePropertySubMute:	
				JARLog("Error : DeviceProperty called for a stream %ld \n",inPropertyID); 
                err = kAudioHardwareUnknownPropertyError;
                break;				
             
            default:
                Print4CharCode("StreamSetProperty unkown request:", inPropertyID);
                err = kAudioHardwareUnknownPropertyError;
                break;
         }
            
        return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
#ifdef kAudioHardwarePlugInInterface2ID
 
OSStatus TJackClient::DeviceStartAtTime(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        AudioDeviceIOProc inProc, 
                                        AudioTimeStamp* ioRequestedStartTime, 
                                        UInt32 inFlags)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceStartAtTime : not yet implemented\n");
	return kAudioHardwareNoError;
}
                                            
//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::DeviceGetNearestStartTime (AudioHardwarePlugInRef inSelf, 
                                                AudioDeviceID inDevice, 
                                                AudioTimeStamp* ioRequestedStartTime, 
                                                UInt32 inFlags)
{
    JARLog("--------------------------------------------------------\n");
    JARLog("DeviceGetNearestStartTime : not yet implemented\n");
	return kAudioHardwareUnsupportedOperationError;
}

#endif

//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::ReadPref()
{
    CFURLRef  prefURL;
    FSRef     prefFolderRef;
    OSErr     err;
    char buf[256];
    char path[256];
    
    err = FSFindFolder(kUserDomain, kPreferencesFolderType, kDontCreateFolder, &prefFolderRef);
    if (err == noErr) {
        prefURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &prefFolderRef);
        if (prefURL) {
            CFURLGetFileSystemRepresentation(prefURL,FALSE,buf,256);
            sprintf(path,"%s/JAS.jpil",buf);
            FILE *prefFile;
            if ((prefFile = fopen(path, "rt"))) {
                int nullo;
                fscanf(prefFile,"\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                        &TJackClient::fInputChannels,
                        &nullo,
                        &TJackClient::fOutputChannels,
                        &nullo,
                        &TJackClient::fAutoConnect,
                        &nullo,
                        &TJackClient::fDefaultInput,
                        &nullo,
                        &TJackClient::fDefaultOutput,
                        &nullo,
                        &TJackClient::fDefaultSystem);                
                fclose(prefFile);
				JARLog("Reading Preferences fInputChannels: %ld fOutputChannels: %ld fAutoConnect: %ld fDefaultInput: %ld fDefaultOutput: %ld fDefaultSystem: %ld\n",
					TJackClient::fInputChannels,TJackClient::fOutputChannels,TJackClient::fAutoConnect,TJackClient::fDefaultInput,TJackClient::fDefaultOutput,TJackClient::fDefaultSystem);
                return true;
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------------------
jack_client_t *  TJackClient::CheckServer(AudioHardwarePlugInRef inSelf)
{
    jack_client_t * client;
    char name [JACK_CLIENT_NAME_LEN];
    sprintf(name, "CA::%ld", (long)inSelf);

    if (client = jack_client_new(name)){
        TJackClient::fBufferSize = jack_get_buffer_size(client);
        TJackClient::fSampleRate = jack_get_sample_rate(client);
        TJackClient::fDeviceRunning = true;
        
		JARLog("CheckServer TJackClient::fBufferSizer %ld\n",TJackClient::fBufferSize);
        JARLog("CheckServer TJackClient::fSampleRate  %f\n",TJackClient::fSampleRate );
         
        OSStatus err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                        TJackClient::fDeviceID, 
                                                        0, 
                                                        0, 
                                                        kAudioDevicePropertyBufferSize);
        JARLog("CheckServer kAudioDevicePropertyBufferSize err %ld\n",err);
    
        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                TJackClient::fDeviceID, 
                                                0, 
                                                0, 
                                                kAudioDevicePropertyBufferFrameSize);
        JARLog("CheckServer kAudioDevicePropertyBufferFrameSize err %ld\n",err);
    
        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                TJackClient::fDeviceID, 
                                                0, 
                                                0, 
                                                kAudioDevicePropertyNominalSampleRate);
		JARLog("CheckServer kAudioDevicePropertyNominalSampleRate err %ld\n",err);
 
        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                TJackClient::fDeviceID, 
                                                0, 
                                                0, 
                                                kAudioDevicePropertyActualSampleRate);
        JARLog("CheckServer kAudioDevicePropertyActualSampleRate err %ld\n",err);

        err = AudioHardwareDevicePropertyChanged(TJackClient::fPlugInRef, 
                                                TJackClient::fDeviceID, 
                                                0, 
                                                0, 
                                                kAudioDevicePropertyDeviceIsAlive);
        JARLog("CheckServer kAudioDevicePropertyIsRunning err %ld\n",err);
        
        return client;
    }else
        return NULL;
}

//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::CheckRunning(AudioHardwarePlugInRef inSelf)
{
    if(TJackClient::fDeviceRunning){ 
        return true;
    }else{
        jack_client_t * client = TJackClient::CheckServer(inSelf);
        if (client) {
            jack_client_close(client);
            return true;
        }else{
            return false;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::Initialize(AudioHardwarePlugInRef inSelf)
{
    OSStatus err = kAudioHardwareNoError;
    char* id_name = bequite_getNameFromPid((int)getpid());

    JARLog("Initialize [inSelf, name] : %ld %s \n", (long)inSelf, id_name);
	
	// Reject "jackd" or "jackdmp" as a possible client (to be impoved if other clients need to be rejected)
	if (strcmp (id_name,"jackd") == 0 || strcmp (id_name,"jackdmp") == 0 ){
		JARLog("Rejected client : %s\n",id_name);
		return noErr;
	}
    
#ifdef kAudioHardwarePlugInInterface2ID  
	UInt32 theSize = 0;
	UInt32 outData = 0;
	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyProcessIsMaster, &theSize, NULL);
	JARLog("kAudioHardwarePropertyProcessIsMaster err theSize %ld %ld\n",err,theSize);
	err = AudioHardwareGetProperty(kAudioHardwarePropertyProcessIsMaster, &theSize, &outData);
	JARLog("kAudioHardwarePropertyProcessIsMaster err outData %ld %ld\n",err,outData);
#endif

    jack_client_t * client; 
    
    if (client = TJackClient::CheckServer(inSelf)){
        
        if (!QueryDevices(client)) return kAudioHardwareBadDeviceError;
        
        const char **ports;
        
        if (!ReadPref()) {
        
            int i = 0;
            if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) != NULL) {
                while (ports[i]) i++;
            }
            
            TJackClient::fInputChannels = max(2,i); // At least 2 channels
            
            i = 0;
            if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) != NULL) {
				while (ports[i]) i++;
            }
            
            TJackClient::fOutputChannels = max(2,i); // At least 2 channels
        }
		
		assert(TJackClient::fInputChannels < MAX_JACK_PORTS);
		assert(TJackClient::fOutputChannels < MAX_JACK_PORTS);
            
        JARLog("fInputChannels %ld \n", TJackClient::fInputChannels);
        JARLog("fOutputChannels %ld \n", TJackClient::fOutputChannels);
		jack_client_close(client);
		
    }else{
        JARLog("jack server not running?\n");
        return kAudioHardwareNotRunningError;
    }        
    
    err = AudioHardwareClaimAudioDeviceID(inSelf, &TJackClient::fDeviceID);
    if (err == kAudioHardwareNoError)
    {
		JARLog("AudioHardwareClaimAudioDeviceID %ld\n", TJackClient::fDeviceID);
		err = AudioHardwareDevicesCreated(inSelf, 1, &TJackClient::fDeviceID);
    }
    if (err == kAudioHardwareNoError)
    {
		for (int i = 0; i < TJackClient::fOutputChannels + TJackClient::fInputChannels; i++) {
			err = AudioHardwareClaimAudioStreamID(inSelf, TJackClient::fDeviceID, &fStreamIDList[i]);
			JARLog("AudioHardwareClaimAudioStreamID %ld\n", (long)TJackClient::fStreamIDList[i]);
			if (err != kAudioHardwareNoError) return err;
		}
		
		err = AudioHardwareStreamsCreated(inSelf, TJackClient::fDeviceID, TJackClient::fOutputChannels + TJackClient::fInputChannels, &TJackClient::fStreamIDList[0]);
		if (err != kAudioHardwareNoError) return err;
		TJackClient::fConnected2HAL = true;
    }
    
    TJackClient::fPlugInRef = inSelf;
     
    if (TJackClient::fDefaultInput) {
        err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultInputDevice, sizeof(UInt32), &TJackClient::fDeviceID);
        if (err != kAudioHardwareNoError) return err;
    }
    
    if (TJackClient::fDefaultOutput) {
        err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultOutputDevice, sizeof(UInt32), &TJackClient::fDeviceID);
        if (err != kAudioHardwareNoError) return err;
    }

    if (TJackClient::fDefaultSystem) {
        err = AudioHardwareSetProperty(kAudioHardwarePropertyDefaultSystemOutputDevice, sizeof(UInt32), &TJackClient::fDeviceID);
        if (err != kAudioHardwareNoError) return err;
    }
	
    return err;
}

//---------------------------------------------------------------------------------------------------------------------------------
OSStatus TJackClient::Teardown(AudioHardwarePlugInRef inSelf)
{
    char* id_name = bequite_getNameFromPid((int)getpid());
	
	KillJackClient(); // In case the client did not correctly quit itself (like iMovie...)
	
	JARLog("Teardown [inSelf, name] : %ld %s \n", (long)inSelf, id_name);
	OSStatus err; 

	err = AudioHardwareStreamsDied(inSelf, TJackClient::fDeviceID, TJackClient::fOutputChannels + TJackClient::fInputChannels, &TJackClient::fStreamIDList[0]);
	JARLog("Teardown : AudioHardwareStreamsDied\n");
	printError(err);
    
    if (TJackClient::fConnected2HAL) {
    	err = AudioHardwareDevicesDied(inSelf, 1, &TJackClient::fDeviceID);
		JARLog("Teardown : AudioHardwareDevicesDied\n");
        printError(err);
		JARLog("Teardown : connection to HAL\n");
    }else{
 		JARLog("Teardown : no connection to HAL\n");
    }

    return kAudioHardwareNoError;
}

//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::ExtractString(char* dst, const char* src, char sep)
{
    int i;
	
	JARLog("ExtractString %s \n", src);
    
    // Look for the first sep character
    for (i = 0; i < strlen(src); i++) if (src[i] == sep) break;
    
    // Move to the character after sep
    i++;
    
    // Copy between the first and second sep character
    for (int j = 0; j < strlen(src); j++) {
    
        assert((j+i) < strlen(src));
        assert(j < JACK_PORT_NAME_LEN);
        
        dst[j] = src[j+i];
        if(src[j+i] == sep) {
            dst[j] = 0; // end string
            return true;
        }
    }
    return false;
}
  
//---------------------------------------------------------------------------------------------------------------------------------
bool TJackClient::QueryDevices(jack_client_t * client)
{
    OSStatus err = noErr;
    UInt32   outSize;
    Boolean  outWritable;
    int      numCoreDevices;
    AudioDeviceID * coreDeviceIDs;
     
    // Get Jack CoreAudio driver name
    const char **ports;
    char port_name [JACK_PORT_NAME_LEN];
    
    port_name[0] = 0; // null string
    
    if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
		JARLog("cannot find any physical capture ports\n");
	}else{
         
		// string of the form "portaudio:Built-in audio :out1"
		if (!ExtractString(port_name,ports[0],':')) {
			JARLog("error : can not extract Jack CoreAudio driver name\n");
		}
      
		JARLog("name %s : len %ld\n", port_name, strlen(port_name));
        free (ports);
    }
    
    if ((ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
        JARLog("cannot find any physical playback ports\n");
    }else{
          
		// string of the form "portaudio:Built-in audio :in1" 
		if (!ExtractString(port_name,ports[0],':')) {
			JARLog("error : can not extract Jack CoreAudio driver name\n");
		}      
    
        JARLog("NAME %s : len %ld\n", port_name, strlen(port_name));
        free (ports);
    }
    
    TJackClient::fCoreAudioDriver = 0;
	
	// Find out how many Core Audio devices there are, if any
    outSize = sizeof(outWritable);
	
	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &outSize, &outWritable);
	JARLog("AudioHardwareGetPropertyInfo : outSize %ld\n", outSize);
   
	if (err != noErr) {
		JARLog("couldn't get info about list of audio devices %ld \n", err);
		printError(err);
		return false;
	}
       
    // Calculate the number of device available
    numCoreDevices = outSize/sizeof(AudioDeviceID);

    // Bail if there aren't any devices
    if (numCoreDevices < 1) {
        JARLog("no Devices Available\n");
        return false;
    }
    
    // Make space for the devices we are about to get
    coreDeviceIDs = (AudioDeviceID *)malloc(outSize);

    // Get an array of AudioDeviceIDs
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &outSize, (void *)coreDeviceIDs);
    if (err != noErr) {
        JARLog("couldn't get list of audio device IDs %ld\n", err);
        return false;
    }

    // Look for the CoreAudio device corresponding to the previously found CoreAudio Jack driver
    char name[256];
    int len = strlen(port_name);
    
    for (int i = 0; i<numCoreDevices; i++) {
    
        err = AudioDeviceGetPropertyInfo(coreDeviceIDs[i], 0, true, kAudioDevicePropertyDeviceName, &outSize, &outWritable);
        
        if (err != noErr) {
			JARLog("couldn't get info about names of audio devices %ld \n", err);
			return false;
        }
       
        err = AudioDeviceGetProperty(coreDeviceIDs[i], 0, true, kAudioDevicePropertyDeviceName, &outSize, (void *)name);
     
        if (err != noErr) {
            JARLog("couldn't get name of device IDs %ld\n", err);
			return false;
        }else {
			JARLog("device name %ld %s \n", coreDeviceIDs[i], name);
		}
         
        if (strncmp(name,port_name,len) == 0) {
            JARLog("found Jack CoreAudio driver : %s \n",name);
			TJackClient::fCoreAudioDriver = coreDeviceIDs[i];
        }
    }
    
    free(coreDeviceIDs);
    return true;
}
 
