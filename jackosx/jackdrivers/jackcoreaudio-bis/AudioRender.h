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

typedef int (*RenderCyclePtr)(void *data,long nframes);

class AudioRender {
    public:
		enum {
			kRenderOn,kRenderOff,kNotConfigured
		};
        AudioRender();
        ~AudioRender();
        bool ConfigureAudioProc(float sampleRate,long bufferSize,int outChannels,int inChannels,char *device_name);
        bool StartAudio();
        bool StopAudio();
		void *cData;
		int cStatus;
		RenderCyclePtr cRenderCallback;
		float **cInBuffers;
        float **cOutBuffers;
		AudioDeviceID cDevice;
        float cSampleRate;
        long cBufferSize;
        int cOutChannels;
        int cInChannels;
		AudioTimeStamp cTime;
    private:
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
};
