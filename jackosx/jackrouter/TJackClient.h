/*
    Copyright © Stefan Werner stefan@keindesign.de, Grame 2003,2004
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
    Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
    grame@rd.grame.fr
*/

#ifndef __TJackClient__
#define __TJackClient__

#include <CoreAudio/CoreAudio.h>
#include <CoreAudio/AudioHardwarePlugin.h>
#include <IOKit/audio/IOAudioTypes.h>
#include <string.h>
#include <jack/jack.h>
#include <map>
#include <list>
#include <string>
#include "bequite.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#define JACK_PORT_NAME_LEN 256
#define JACK_CLIENT_NAME_LEN 256
#define  MAX_JACK_PORTS 128

void JARLog(char *fmt,...);

enum {
    kAudioDevicePropertyGetJackClient  = 'jasg', 
    kAudioDevicePropertyReleaseJackClient  = 'jasr'
};

// The IOProc context

struct TProcContext {

    void* fContext;
    bool fStatus;
	bool fStreamUsage;
	bool fInput[MAX_JACK_PORTS];
	bool fOutput[MAX_JACK_PORTS];
	
    TProcContext(void* context) 
	{
		fContext = context; 
		fStatus = false;
		fStreamUsage = false;
		for (int i = 0; i<MAX_JACK_PORTS; i++) {
			fInput[i] = fOutput[i] = false;
		}
	}
};


// The CoreAudio/Jack client

class TJackClient {

    private:
	
		jack_client_t * fClient;	// Jack client 
        
        jack_port_t * fInputPortList[MAX_JACK_PORTS];  // Jack input ports
        jack_port_t * fOutputPortList[MAX_JACK_PORTS];  // Jack output ports
         
        AudioBufferList* fInputList;	// CoreAudio input buffers
        AudioBufferList* fOutputList;	// CoreAudio output buffers
        
        float** fOuputListMixing;	// Buffers for mixing
        
        map<AudioDeviceIOProc,TProcContext> fAudioIOProcList;   // Table of IOProc 
         
        long fProcRunning;	// Counter of running IOProc
        long fExternalClientNum;	// Counter of external clients (Jack plug-ins)
		long fInternalClientNum;	// Counter of internal client	
		
		// Global state 
        static TJackClient* fJackClient;
        static list<pair<string,string> > fConnections;  // Connections list
        
        static long fBufferSize;
        static long fInputChannels;
        static long fOutputChannels;
        static float fSampleRate;
        static bool fAutoConnect;
        static bool fDefaultInput;
        static bool fDefaultOutput;
        static bool fDefaultSystem;
		
        static string fDeviceName;
        static string fStreamName;
        static string fDeviceManufacturer;
        static bool fDeviceRunning;
        static bool fConnected2HAL;
	     
        static AudioDeviceID fDeviceID;
        static AudioStreamID fStreamIDList[128];
        
        static AudioDeviceID fCoreAudioDriver;		// The CoreAudio driver currently loaded by Jack
        static AudioHardwarePlugInRef fPlugInRef;
        
        static bool ExtractString(char* dst, const char* src, char sep);
		static void SetTime(AudioTimeStamp* timeVal, long curTime, UInt64 time);

    public:
     
        TJackClient();
        virtual ~TJackClient();
           
        static int Process(jack_nframes_t nframes, void *arg);
		
        static TJackClient* GetJackClient(); 
		static void ClearJackClient();
		static void KillJackClient();
		
		static bool fDebug;
	    
        bool Open();  
        void Close();
		
		bool AllocatePorts();
		void DisposePorts();
          
        bool AddIOProc(AudioDeviceIOProc proc, void* context); 
        bool RemoveIOProc(AudioDeviceIOProc proc);
        void ClearIOProc() {fAudioIOProcList.clear();}
        
        long GetProcNum() {return fAudioIOProcList.size();}
       
        bool Activate();
        bool Desactivate();
		bool AutoConnect();
        
        void Start(AudioDeviceIOProc proc);
        void Stop(AudioDeviceIOProc proc);
        
		static void CheckLastRef();
        static void IncRefInternal();
        static void DecRefInternal();
        static void IncRefExternal();
        static void DecRefExternal();
        
        bool IsRunning() {return fProcRunning > 0;}
        void IncRunning() {fProcRunning++;}
        void DecRunning() {fProcRunning--; if (fProcRunning < 0) fProcRunning = 0;}
		
		void StopRunning() {fProcRunning = 0;}
           
        static bool QueryDevices(jack_client_t * client);
        static bool ReadPref();
        static void Shutdown(void *arg);
        static jack_client_t * CheckServer(AudioHardwarePlugInRef inSelf);
        static bool CheckRunning(AudioHardwarePlugInRef inSelf);
        
        void SaveConnections();
        bool RestoreConnections();
		
		// Plug-in API
  
        static OSStatus	Initialize(AudioHardwarePlugInRef inSelf);
        static OSStatus Teardown(AudioHardwarePlugInRef inSelf);
     
        static OSStatus DeviceAddIOProc(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        AudioDeviceIOProc proc, 
                                        void* data);
                                        
        static OSStatus DeviceRemoveIOProc(AudioHardwarePlugInRef inSelf,
                                      	AudioDeviceID inDevice, 
                                        AudioDeviceIOProc proc);
                                        
        static OSStatus DeviceStart(AudioHardwarePlugInRef inSelf,
                                    AudioDeviceID inDevice, 
                                    AudioDeviceIOProc proc);
                                    
        static OSStatus DeviceStop(AudioHardwarePlugInRef inSelf,
                                    AudioDeviceID inDevice, 
                                    AudioDeviceIOProc proc);
                                    
        static OSStatus DeviceRead(AudioHardwarePlugInRef inSelf, 
                                    AudioDeviceID inDevice, 
                                    const AudioTimeStamp* inStartTime, 
                                    AudioBufferList* outData);

        static OSStatus DeviceGetCurrentTime(AudioHardwarePlugInRef inSelf,
                                            AudioDeviceID inDevice,
                                            AudioTimeStamp* outTime);
                                            
        static OSStatus DeviceTranslateTime(AudioHardwarePlugInRef inSelf,
                                            AudioDeviceID inDevice,
                                            const AudioTimeStamp* inTime,
                                            AudioTimeStamp* outTime);

        static OSStatus	DeviceGetPropertyInfo(AudioHardwarePlugInRef inSelf, 
                                                AudioDeviceID inDevice, 
                                                UInt32 inChannel, 
                                                Boolean isInput, 
                                                AudioDevicePropertyID inPropertyID, 
                                                UInt32* outSize, 
                                                Boolean* outWritable);
                                                
        static OSStatus DeviceGetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        UInt32 inChannel, 
                                        Boolean isInput, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32* ioPropertyDataSize, 
                                        void* outPropertyData);
                                        
        static OSStatus DeviceSetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        const AudioTimeStamp* inWhen, 
                                        UInt32 inChannel, 
                                        Boolean isInput, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32 inPropertyDataSize, 
                                        const void* inPropertyData);

        static OSStatus StreamGetPropertyInfo(AudioHardwarePlugInRef inSelf, 
                                            AudioStreamID inStream, 
                                            UInt32 inChannel, 
                                            AudioDevicePropertyID inPropertyID,
                                            UInt32* outSize, 
                                            Boolean* outWritable);

        static OSStatus StreamGetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioStreamID inStream, 
                                        UInt32 inChannel, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32* ioPropertyDataSize, 
                                        void* outPropertyData);
                                        
        static OSStatus	StreamSetProperty(AudioHardwarePlugInRef inSelf, 
                                        AudioStreamID inStream, 
                                        const AudioTimeStamp* inWhen, 
                                        UInt32 inChannel, 
                                        AudioDevicePropertyID inPropertyID, 
                                        UInt32 inPropertyDataSize, 
                                        const void* inPropertyData);
    // Version 2 Methods QT 6.4
    #ifdef kAudioHardwarePlugInInterface2ID
    
        static 	OSStatus DeviceStartAtTime(AudioHardwarePlugInRef inSelf, 
                                        AudioDeviceID inDevice, 
                                        AudioDeviceIOProc inProc, 
                                        AudioTimeStamp* ioRequestedStartTime, 
                                        UInt32 inFlags);
                                            
        static OSStatus	DeviceGetNearestStartTime (AudioHardwarePlugInRef inSelf, 
                                                AudioDeviceID inDevice, 
                                                AudioTimeStamp* ioRequestedStartTime, 
                                                UInt32 inFlags);
    #endif

 };

     
#ifdef __cplusplus
}
#endif


#endif
