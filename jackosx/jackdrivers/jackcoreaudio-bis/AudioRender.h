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
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>


#define	Print4CharCode(msg, c)	{														\
	UInt32 __4CC_number = (c);		\
		char __4CC_string[5];		\
	memcpy(__4CC_string, &__4CC_number, 4);	\
	__4CC_string[4] = 0;			\
	printf("%s'%s'\n", (msg), __4CC_string);\
}

typedef int (*JackRunCyclePtr)(void * driver,long bufferSize);

class AudioRender {
    public:
        AudioRender(float sampleRate,float virtual_sampleRate,long bufferSize,int inChannels, int outChannels,char *device);
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
		float vir_SampleRate;
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
        static AudioRender *theRender;
		AudioUnit device_au;
		AudioBufferList *inputsBufs;
};
