/*
 *  AudioRender.h
 *  Under LGPL License.
 *  This code is part of Panda AUHAL driver.
 *  http://xpanda.sourceforge.net
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

typedef int (*RenderCyclePtr)(void *data,long nframes,float **inBuffers,float **outBuffers);

class AudioRender {
    public:
        AudioRender();
        ~AudioRender();
        bool ConfigureAudioProc(float sampleRate,long bufferSize,int outChannels,int inChannels,char *device_name);
        bool StartAudio();
        bool StopAudio();
		int InputChannelsCount() { int res = cInChannels; return res; }
		int OutputChannelsCount() { int res = cOutChannels; return res; }
		float SampleRate() { float res = cSampleRate; return res; }
		long BufferFrames() { long res = cBufferSize; return res; }
		AudioTimeStamp GetAudioTimeStamp() { AudioTimeStamp res; memcpy(&res,&cTime,sizeof(AudioTimeStamp)); return res; }
		AudioDeviceID DeviceID() { AudioDeviceID res = cDevice; return res; }
		void SetHostData(void *data) { cData = data; }
		void * GetHostData() { return cData; }
		void SetRenderCallback(RenderCyclePtr fun) { cRenderCallback = fun; }
    private:
		enum {
			kRenderOn,kRenderOff,kNotConfigured
		};
		static OSStatus	MyRender(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
				AudioBufferList 			*ioData);
		static OSStatus	MyRenderInput(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
				AudioBufferList 			*ioData);
		AudioUnit cDeviceAu;
		AudioBufferList *cInputsBufs;
		RenderCyclePtr cRenderCallback;
		int cStatus;
		void *cData;
		float **cInBuffers;
        float **cOutBuffers;
		AudioDeviceID cDevice;
        float cSampleRate;
        long cBufferSize;
        int cOutChannels;
        int cInChannels;
		AudioTimeStamp cTime;
};
