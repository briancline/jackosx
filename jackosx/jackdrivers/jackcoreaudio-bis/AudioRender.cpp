 /*
 *  AudioRender.cpp
 *  Under Artistic License.
 *  This code is part of Panda framework (moduleloader.cpp)
 *  http://xpanda.sourceforge.net
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

#include "AudioRender.h"
#include <unistd.h>
#define DEBUG 1
//#undef DEBUG

float AudioRender::gSampleRate = 0.0;
long AudioRender::gBufferSize = 0;
int AudioRender::gInputChannels = 0;
int AudioRender::gOutputChannels = 0;
AudioRender *AudioRender::theRender = NULL;
bool AudioRender::isProcessing = false;
const AudioTimeStamp *AudioRender::gTime;

extern "C" void JCALog(char *fmt,...) {
#ifdef DEBUG
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"JCA: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
    if (!inDesc) {
        JCALog ("Can't print a NULL desc!\n");
        return;
    }
    
    JCALog ("- - - - - - - - - - - - - - - - - - - -\n");
    JCALog ("  Sample Rate:%f\n", inDesc->mSampleRate);
    JCALog("  Format ID:%.*s\n", (int) sizeof(inDesc->mFormatID), (char*)&inDesc->mFormatID);
    JCALog ("  Format Flags:%lX\n", inDesc->mFormatFlags);
    JCALog ("  Bytes per Packet:%ld\n", inDesc->mBytesPerPacket);
    JCALog ("  Frames per Packet:%ld\n", inDesc->mFramesPerPacket);
    JCALog ("  Bytes per Frame:%ld\n", inDesc->mBytesPerFrame);
    JCALog ("  Channels per Frame:%ld\n", inDesc->mChannelsPerFrame);
    JCALog ("  Bits per Channel:%ld\n", inDesc->mBitsPerChannel);
    JCALog ("- - - - - - - - - - - - - - - - - - - -\n");
}

OSStatus	AudioRender::MyRender(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
				AudioBufferList 			*ioData)

{
	
	AudioRender *x = (AudioRender*)inRefCon;
	
	AudioUnitRender(x->device_au,ioActionFlags,inTimeStamp,1,inNumberFrames,x->inputsBufs);
		
	for(UInt32 i=0;i<ioData->mNumberBuffers;i++) {
		x->outBuffers[i] = (float*)ioData->mBuffers[i].mData;
	}
	
	x->f_JackRunCycle(x->jackData,inNumberFrames);
	
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
	
	AudioUnitRender(x->device_au,ioActionFlags,inTimeStamp,1,inNumberFrames,x->inputsBufs);
	
	x->f_JackRunCycle(x->jackData,inNumberFrames);
	
	return noErr;
}


AudioRender::AudioRender(float sampleRate,float virtual_sampleRate,long bufferSize,int inChannels, int outChannels, char *device) : 	vSampleRate(sampleRate),vBufferSize(bufferSize) 
{
    inBuffers=NULL;
    outBuffers=NULL;
	vir_SampleRate = virtual_sampleRate;
	
    status = ConfigureAudioProc(sampleRate,bufferSize,outChannels,inChannels,device);
	
	AudioRender::gSampleRate = vSampleRate;
    AudioRender::gBufferSize = vBufferSize;
    AudioRender::gInputChannels = vInChannels;
    AudioRender::gOutputChannels = vChannels;
    AudioRender::theRender = this;
    isProcessing = false;
	
    if(status) {
        inBuffers = (float**)malloc(sizeof(float*)*vInChannels);
        outBuffers = (float**)malloc(sizeof(float*)*vChannels);
		inputsBufs = (AudioBufferList*)malloc(offsetof(AudioBufferList,mBuffers[vInChannels]));
		inputsBufs->mNumberBuffers = vInChannels;
		for(int i=0;i<vInChannels;i++) {
			inBuffers[i] = (float*)malloc(sizeof(float)*vBufferSize);
			inputsBufs->mBuffers[i].mData = inBuffers[i];
			inputsBufs->mBuffers[i].mNumberChannels = 1;
		}
		JCALog("AudioRender created.\n");
		JCALog("HALUnit version.\n");
	} else JCALog("error while creating AudioRender.\n");
}

AudioRender::~AudioRender() {
	if(status) {
		if(isProcessing) AudioOutputUnitStop (device_au);
		CloseComponent(device_au);
		status = false;
		for(int i =0;i<vInChannels;i++) {
			free(inBuffers[i]);
		}
		free(inputsBufs);
		free(inBuffers);
		free(outBuffers);
	}
}

bool AudioRender::ConfigureAudioProc(float sampleRate,long bufferSize,int channels,int inChannels,char *device) {

OSStatus err,error;
    UInt32 size;
    Boolean isWritable;

	ComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_HALOutput,kAudioUnitManufacturer_Apple, 0, 0};
	Component HALOutput = FindNextComponent(nil, &cd);
	
	err = OpenAComponent(HALOutput, &device_au);

	err = AudioUnitInitialize(device_au);
		
	JCALog("Wanted DEVICE: %s\n",device);
	
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
    if(err!=noErr) return false;
    
    int manyDevices = size/sizeof(AudioDeviceID);
    
    AudioDeviceID devices[manyDevices];
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices);
    if(err!=noErr) return false;
	
	bool found = false;
    
    for(int i=0;i<manyDevices;i++) {
        size = sizeof(char)*256;
        char name[256];
        err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name);
		JCALog("Read DEVICE: %s\n",name);
        if(err!=noErr) return false;
		if(strncmp(device,name,strlen(device))==0) {  
			JCALog("Found DEVICE: %s %ld\n",name,device);
			vDevice = devices[i];
			found = true;
		}
    }
	
	if(!found) { JCALog("Cannot find device \"%s\".\n",device); return false; }

	error = AudioUnitSetProperty(device_au,kAudioOutputUnitProperty_CurrentDevice,kAudioUnitScope_Global,0,&vDevice,sizeof(AudioDeviceID));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_CurrentDevice\n"); return false; }

	UInt32 data = 1;
	error = AudioUnitSetProperty(device_au,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output,0,&data,sizeof(data));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO\n"); return false; }
	
	if(inChannels>0) {

		data = 1;
		error = AudioUnitSetProperty(device_au,kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,1,&data,sizeof(data));
		if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO2\n"); return false; }
		
	}
	
	size = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription SR;
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&SR);
    if(err!=noErr) return false;

	vSampleRate = (float)SR.mSampleRate;
    
    if((float)SR.mSampleRate!=sampleRate) {
        JCALog("trying to set a new sample rate\n");
        UInt32 theSize = sizeof(AudioStreamBasicDescription);
        SR.mSampleRate = (Float64)sampleRate;
        err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyStreamFormat,theSize,&SR);
        if(err!=noErr) JCALog("Cannot set a new sample rate\n");
        else {
            size = sizeof(AudioStreamBasicDescription);
            AudioStreamBasicDescription newCheckSR;
            err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&newCheckSR);
            if(err!=noErr) return false;
            vSampleRate = (float)newCheckSR.mSampleRate;
        }
    }
	
	UInt32 bufFrame;
    size = sizeof(UInt32);
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&bufFrame);
    if(err!=noErr) return false;
            
    vBufferSize = (long) bufFrame;
    
    if((long)bufFrame!=bufferSize) { 
        JCALog("I'm trying to set a new buffer size\n");
        UInt32 theSize = sizeof(UInt32);
        UInt32 newBufferSize = (UInt32) bufferSize;
        err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufferSize);
        if(err!=noErr) JCALog("Cannot set a new buffer size\n");
        else {	
            UInt32 newBufFrame;
            size = sizeof(UInt32);
            err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&newBufFrame);
            if(err!=noErr) return false;
            vBufferSize = (long)newBufFrame;
        }
    }
    
    JCALog("BUFFER SIZE: %ld\n",vBufferSize);
	
	error = AudioUnitSetProperty (device_au,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,(UInt32*)&bufferSize,sizeof (UInt32));
	if(error) { JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice\n"); return false; }
	
	vBufferSize = bufferSize;

	Boolean writable = false;
	UInt32 propsize;
	
	error = AudioUnitGetPropertyInfo(device_au,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Input,1, &propsize, &writable);
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap-INFO 1\n");
	
	long nChannels = propsize / sizeof(SInt32);
	
	
	error = AudioUnitGetPropertyInfo(device_au,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Output,0, &propsize, &writable);
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap-INFO 0\n");

	long out_nChannels = propsize / sizeof(SInt32);
	
	
	
	if(channels>out_nChannels) { JCALog("This device hasn't required output channels.\n"); return false; }
	if(inChannels>nChannels) { JCALog("This device hasn't required input channels.\n"); return false; }
	
	vChannels = channels;
	vInChannels = inChannels;
	
	JCALog("INPUT CHANNELS: %ld\n",vInChannels);
	JCALog("OUTPUT CHANNELS: %ld\n",vChannels);
	
	if(vChannels<out_nChannels) {
		SInt32 chanArr[out_nChannels];
		for(int i=0;i<out_nChannels;i++) {
			chanArr[i] = -1;
		}
		for(int i=0;i<vChannels;i++) {
			chanArr[i] = i;
		}
		error = AudioUnitSetProperty(device_au,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Output,0,&chanArr[0],sizeof(SInt32)*out_nChannels);
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 0\n");
	}
	if(vInChannels<nChannels) {
		SInt32 chanArr[nChannels];
		for(int i=0;i<nChannels;i++) {
			chanArr[i] = -1;
		}
		for(int i=0;i<vInChannels;i++) {
			chanArr[i] = i;
		}
		AudioUnitSetProperty(device_au,kAudioOutputUnitProperty_ChannelMap,kAudioUnitScope_Input,1,&chanArr[0],sizeof(SInt32)*nChannels);
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioOutputUnitProperty_ChannelMap 1\n");
	}

	Float64 strSampleRate = 0;
	if(vir_SampleRate != 0) {
		JCALog("trying to set a virtual sample rate.\n");
		strSampleRate = vir_SampleRate;
	} else strSampleRate = sampleRate;
	
	AudioStreamBasicDescription dstFormat;
	dstFormat.mSampleRate = strSampleRate;
    dstFormat.mFormatID = kAudioFormatLinearPCM;
    dstFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian |
                                kLinearPCMFormatFlagIsNonInterleaved |
                                kLinearPCMFormatFlagIsPacked |
                                kLinearPCMFormatFlagIsFloat;
    dstFormat.mBytesPerPacket = sizeof(float);
    dstFormat.mFramesPerPacket = 1;
    dstFormat.mBytesPerFrame = sizeof(float);
    dstFormat.mChannelsPerFrame = vChannels;
    dstFormat.mBitsPerChannel = 32;
	
	error = AudioUnitSetProperty(device_au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&dstFormat,sizeof(AudioStreamBasicDescription));
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input\n");
	
	dstFormat.mChannelsPerFrame = vInChannels;
		
	error = AudioUnitSetProperty(device_au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,1,&dstFormat,sizeof(AudioStreamBasicDescription));
	if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output\n");
	
	/*
	if(vir_SampleRate!=0) {
		UInt32 maxQuality = 127;
		error = AudioUnitSetProperty(device_au,kAudioUnitProperty_RenderQuality,kAudioUnitScope_Global,0,&maxQuality,sizeof(UInt32));
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_RenderQuality\n");
		error = AudioUnitSetProperty(device_au,kAudioUnitProperty_RenderQuality,kAudioUnitScope_Global,1,&maxQuality,sizeof(UInt32));
		if(error) JCALog("error: calling AudioUnitSetProperty - kAudioUnitProperty_RenderQuality\n");
	}
	*/
	
	
	if(vInChannels>0 && vChannels==0) {
		AURenderCallbackStruct output;
		output.inputProc = MyRenderInput;
		output.inputProcRefCon = this;

		error = AudioUnitSetProperty (device_au, kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Output,1,&output,sizeof(output));
		if (error) { JCALog ("AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 1\n"); return false; }
	} else {
		AURenderCallbackStruct output;
		output.inputProc = MyRender;
		output.inputProcRefCon = this;

		error = AudioUnitSetProperty (device_au, kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&output,sizeof(output));
		if (error) { JCALog ("AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 0\n"); return false; }
	}
	
    return true;
	
}

bool AudioRender::StartAudio() {
	if(status) {
		OSStatus err = AudioOutputUnitStart (device_au);
		if(err!=noErr) return false;
		AudioRender::isProcessing = true;
		return true;
	} 
    return false;
}

bool AudioRender::StopAudio() {
	if(status) {
		OSStatus err = AudioOutputUnitStop (device_au);
		if(err!=noErr) return false;
		AudioRender::isProcessing = false;
		return true;
	}
    return false;
}

float **AudioRender::getADC() {
    if(AudioRender::theRender==NULL) return NULL;
    return AudioRender::theRender->inBuffers;
}
float **AudioRender::getDAC() {
    if(AudioRender::theRender==NULL) return NULL;
    return AudioRender::theRender->outBuffers;
}




