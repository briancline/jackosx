/*
 *  AudioRender.h
 *  Under Artistic License.
 *  This code is part of Panda framework (moduleloader.hpp)
 *  http://xpanda.sourceforge.net
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include <CoreAudio/CoreAudio.h>

typedef int (*JackRunCyclePtr)(void * driver,long bufferSize);

class AudioRender {
    public:
        AudioRender(float sampleRate,long bufferSize,int inChannels, int outChannels,char *device);
        ~AudioRender();
        bool ConfigureAudioProc(float sampleRate,long bufferSize,int channels,int inChannels, char *device);
        bool StartAudio();
        bool StopAudio();
		void *jackData;
		bool status;
		JackRunCyclePtr f_JackRunCycle;
		float **inBuffers;
        float **outBuffers;
		AudioDeviceID vDevice;
        float vSampleRate;
        long vBufferSize;
        int vChannels;//output channels
        int vInChannels; //input chennels
		static float **getADC();
        static float **getDAC();
        static float gSampleRate;
        static long gBufferSize;
        static int gInputChannels;
        static int gOutputChannels;
        static bool isProcessing;
        static const AudioTimeStamp *gTime;
    private:
        static OSStatus process(AudioDeviceID inDevice,const AudioTimeStamp* inNow,const AudioBufferList* inInputData,const AudioTimeStamp* 
					inInputTime,AudioBufferList* outOutputData,const AudioTimeStamp* inOutputTime,void* inClientData);
        static AudioRender *theRender;
};
