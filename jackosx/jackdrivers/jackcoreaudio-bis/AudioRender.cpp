 /*
 *  AudioRender.cpp
 *  Under LGPL License.
 *  This code is part of Panda AUHAL driver.
 *  http://xpanda.sourceforge.net
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

#include "AudioRender.h"
#include <unistd.h>
#define DEBUG 1

extern "C" void JCALog(char *fmt,...) {
#ifdef DEBUG
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"JCA: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

OSStatus	AudioRender::MyRender(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
				AudioBufferList 			*ioData)

{
	AudioRender *x = (AudioRender*)inRefCon;
	
	memcpy(&x->cTime,inTimeStamp,sizeof(AudioTimeStamp));
	
	AudioUnitRender(x->cDeviceAu,ioActionFlags,inTimeStamp,1,inNumberFrames,x->cInputsBufs);
		
	for(UInt32 i=0;i<ioData->mNumberBuffers;i++) {
		x->cOutBuffers[i] = (float*)ioData->mBuffers[i].mData;
	}
	
	x->cRenderCallback(x->cData,inNumberFrames,x->cInBuffers,x->cOutBuffers);
	
	return noErr;
}

OSStatus	AudioRender::MyRenderInput(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
				AudioBufferList 			*ioData)

{
	AudioRender *x = (AudioRender*)inRefCon;

	memcpy(&x->cTime,inTimeStamp,sizeof(AudioTimeStamp));
	
	AudioUnitRender(x->cDeviceAu,ioActionFlags,inTimeStamp,1,inNumberFrames,x->cInputsBufs);
	
	x->cRenderCallback(x->cData,inNumberFrames,x->cInBuffers,x->cOutBuffers);
	
	return noErr;
}


AudioRender::AudioRender() {
    cInBuffers=NULL;
    cOutBuffers=NULL;
	cStatus = kNotConfigured;
}

AudioRender::~AudioRender() {
	if(cStatus==kRenderOn) {
		AudioOutputUnitStop(cDeviceAu);
		cStatus = kRenderOff;
	}
	if(cStatus==kRenderOff) {
		AudioUnitUninitialize(cDeviceAu);
		CloseComponent(cDeviceAu);
		cStatus = kNotConfigured;
		for(int i=0;i<cInChannels;i++) {
			if(cInBuffers[i]) free(cInBuffers[i]);
		}
		if(cInputsBufs) free(cInputsBufs);
		if(cInBuffers) free(cInBuffers);
		if(cOutBuffers) free(cOutBuffers);
	}
}

bool AudioRender::ConfigureAudioProc(float sampleRate,long bufferSize,int outChannels,int inChannels,char *device_name) {

	OSStatus err = noErr;
	ComponentResult error = noErr;
    UInt32 size = 0;
    Boolean isWritable = FALSE;

	ComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_HALOutput,kAudioUnitManufacturer_Apple, 0, 0};
	Component HALOutput = FindNextComponent(NULL,&cd);
	
	error = OpenAComponent(HALOutput,&cDeviceAu);

	error = AudioUnitInitialize(cDeviceAu); // maybe at the end of the function.
	
	if(strcmp(device_name,"DEFAULT")==0) {
		if(outChannels>0) err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,&size,&cDevice);
		else err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,&size,&cDevice);
		if(err!=noErr) return false;
	} else {
		JCALog("Wanted DEVICE: %s\n",device_name);
	
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
		if(err!=noErr) return false;
    
		int manyDevices = size/sizeof(AudioDeviceID);
    
		AudioDeviceID devices[manyDevices];
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices[0]);
		if(err!=noErr) return false;
	
		bool found = false;
		char name[60];
	
		for(int i=0;i<manyDevices;i++) {
			size = sizeof(char)*60;
			bzero(&name[0],size);
			err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name[0]);
			JCALog("Reading device: %s\n",name);
			if(err!=noErr) return false;
			if(strncmp(device_name,name,strlen(device_name))==0) {  
				JCALog("Found device: %s %ld\n",name,device_name);
				cDevice = devices[i];
				found = true;
			}
		}
	
		if(!found) { 
			JCALog("Cannot find device \"%s\".\n",device_name); 
			return false;
		}
	}
	
	error = AudioUnitSetProperty(cDeviceAu,kAudioOutputUnitProperty_CurrentDevice,kAudioUnitScope_Global,0,&cDevice,sizeof(AudioDeviceID));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_CurrentDevice\n"); return false; }

	UInt32 data = 1;
	
	error = AudioUnitSetProperty(cDeviceAu,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output,0,&data,sizeof(data));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output\n"); return false; }
	
	if(inChannels>0) {
		data = 1;
		error = AudioUnitSetProperty(cDeviceAu,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,1,&data,sizeof(data));
		if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input\n"); return false; }
	}
	
	size = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription SR;
    err = AudioDeviceGetProperty(cDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&SR);
    if(err!=noErr) return false;

	cSampleRate = (float)SR.mSampleRate;
    
    if(cSampleRate!=sampleRate) {
        JCALog("trying to set a new sample rate\n");
        UInt32 theSize = sizeof(AudioStreamBasicDescription);
        SR.mSampleRate = (Float64)sampleRate;
        err = AudioDeviceSetProperty(cDevice,NULL,0,false,kAudioDevicePropertyStreamFormat,theSize,&SR);
        if(err!=noErr) { JCALog("Cannot set a new sample rate\n"); return false; }
        else {
            size = sizeof(AudioStreamBasicDescription);
            AudioStreamBasicDescription newCheckSR;
            err = AudioDeviceGetProperty(cDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&newCheckSR);
            if(err==noErr) cSampleRate = (float)newCheckSR.mSampleRate;
        }
    }
	
	JCALog("Sample Rate: %f.\n",cSampleRate);
	
	UInt32 bufFrames;
    size = sizeof(UInt32);
    err = AudioDeviceGetProperty(cDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&bufFrames);
    if(err!=noErr) return false;
            
    cBufferSize = (long)bufFrames;
    
    if(cBufferSize!=bufferSize) { 
        JCALog("I'm trying to set a new buffer size\n");
        UInt32 theSize = sizeof(UInt32);
        UInt32 newBufferSize = (UInt32) bufferSize;
        err = AudioDeviceSetProperty(cDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufferSize);
        if(err!=noErr) { JCALog("Cannot set a new buffer size\n"); return false; }
        else {	
            UInt32 newBufFrame;
            size = sizeof(UInt32);
            err = AudioDeviceGetProperty(cDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&newBufFrame);
            if(err==noErr) cBufferSize = (long)newBufFrame;
        }
    }
    
    JCALog("Buffer Size: %ld\n",cBufferSize);
	
	error = AudioUnitSetProperty (cDeviceAu,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,(UInt32*)&cBufferSize,sizeof(UInt32));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice\n"); return false; }
	
	error = AudioUnitGetPropertyInfo(cDeviceAu,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Input,1,&size,&isWritable);
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap-INFO 1\n");
	
	long in_nChannels = size / sizeof(SInt32);
	
	error = AudioUnitGetPropertyInfo(cDeviceAu,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Output,0, &size,&isWritable);
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap-INFO 0\n");

	long out_nChannels = size / sizeof(SInt32);
	
	if(outChannels>out_nChannels) { JCALog("This device hasn't required output channels.\n"); return false; }
	if(inChannels>in_nChannels) { JCALog("This device hasn't required input channels.\n"); return false; }
	
	cOutChannels = outChannels;
	cInChannels = inChannels;
	
	JCALog("Input Channels: %ld\n",cInChannels);
	JCALog("Output Channels: %ld\n",cOutChannels);
	
	if(cOutChannels<out_nChannels) {
		SInt32 chanArr[out_nChannels];
		for(int i=0;i<out_nChannels;i++) {
			chanArr[i] = -1;
		}
		for(int i=0;i<cOutChannels;i++) {
			chanArr[i] = i;
		}
		error = AudioUnitSetProperty(cDeviceAu,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Output,0,&chanArr[0],sizeof(SInt32)*out_nChannels);
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 0\n");
	}
	
	if(cInChannels<in_nChannels) {
		SInt32 chanArr[in_nChannels];
		for(int i=0;i<in_nChannels;i++) {
			chanArr[i] = -1;
		}
		for(int i=0;i<cInChannels;i++) {
			chanArr[i] = i;
		}
		AudioUnitSetProperty(cDeviceAu,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Input,1,&chanArr[0],sizeof(SInt32)*in_nChannels);
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 1\n");
	}
	
	AudioStreamBasicDescription dstFormat;
	dstFormat.mSampleRate = cSampleRate;
    dstFormat.mFormatID = kAudioFormatLinearPCM;
    dstFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsNonInterleaved | kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsFloat;
    dstFormat.mBytesPerPacket = sizeof(float);
    dstFormat.mFramesPerPacket = 1;
    dstFormat.mBytesPerFrame = sizeof(float);
    dstFormat.mChannelsPerFrame = cOutChannels;
    dstFormat.mBitsPerChannel = 32;
	
	error = AudioUnitSetProperty(cDeviceAu,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&dstFormat,sizeof(AudioStreamBasicDescription));
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input\n");
	
	dstFormat.mChannelsPerFrame = cInChannels;
		
	error = AudioUnitSetProperty(cDeviceAu,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,1,&dstFormat,sizeof(AudioStreamBasicDescription));
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output\n");
	
	if(cInChannels>0 && cOutChannels==0) {
		AURenderCallbackStruct output;
		output.inputProc = MyRenderInput;
		output.inputProcRefCon = this;
		error = AudioUnitSetProperty (cDeviceAu, kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Output,1,&output,sizeof(output));
		if (error) { JCALog ("AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 1\n"); return false; }
	} else {
		AURenderCallbackStruct output;
		output.inputProc = MyRender;
		output.inputProcRefCon = this;
		error = AudioUnitSetProperty (cDeviceAu, kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&output,sizeof(output));
		if (error) { JCALog ("AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 0\n"); return false; }
	}
	
	cInBuffers = (float**)malloc(sizeof(float*)*cInChannels);
	cOutBuffers = (float**)malloc(sizeof(float*)*cOutChannels);
	cInputsBufs = (AudioBufferList*)malloc(offsetof(AudioBufferList,mBuffers[cInChannels]));
	cInputsBufs->mNumberBuffers = cInChannels;
	for(int i=0;i<cInChannels;i++) {
		cInBuffers[i] = (float*)malloc(sizeof(float)*cBufferSize);
		cInputsBufs->mBuffers[i].mData = cInBuffers[i];
		cInputsBufs->mBuffers[i].mNumberChannels = 1;
	}
	
	cStatus = kRenderOff;
	
    return true;
	
}

bool AudioRender::StartAudio() {
	if(cStatus==kRenderOff) {
		OSStatus err = AudioOutputUnitStart(cDeviceAu);
		if(err!=noErr) return false;
		cStatus = kRenderOn;
		return true;
	} 
    return false;
}

bool AudioRender::StopAudio() {
	if(cStatus==kRenderOn) {
		OSStatus err = AudioOutputUnitStop(cDeviceAu);
		if(err!=noErr) return false;
		cStatus = kRenderOff;
		return true;
	}
    return false;
}




