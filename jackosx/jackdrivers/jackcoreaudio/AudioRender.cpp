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

float AudioRender::gSampleRate = 0.0;
long AudioRender::gBufferSize = 0;
int AudioRender::gInputChannels = 0;
int AudioRender::gOutputChannels = 0;
AudioRender *AudioRender::theRender = NULL;
bool AudioRender::isProcessing = false;
const AudioTimeStamp *AudioRender::gTime;

AudioRender::AudioRender(float sampleRate,long bufferSize,int inChannels, int outChannels, char *device) : 	vSampleRate(sampleRate),vBufferSize(bufferSize) 
{
    inBuffers=NULL;
    outBuffers=NULL;
    status = ConfigureAudioProc(sampleRate,bufferSize,outChannels,inChannels,device);
	
	AudioRender::gSampleRate = vSampleRate;
    AudioRender::gBufferSize = vBufferSize;
    AudioRender::gInputChannels = vInChannels;
    AudioRender::gOutputChannels = vChannels;
    AudioRender::theRender = this;
    isProcessing = false;
	
    if(status) {
        inBuffers = (float**)malloc(sizeof(float*)*vChannels);
        outBuffers = (float**)malloc(sizeof(float*)*vChannels);
        for(int i =0;i< vChannels;i++) {
            inBuffers[i] = (float*)malloc(sizeof(float)*vBufferSize);
        }
        for(int i =0;i< vChannels;i++) {
            outBuffers[i] = (float*)malloc(sizeof(float)*vBufferSize);
            memset(outBuffers[i], 0, vBufferSize*sizeof(float));
        }
    }
}

AudioRender::~AudioRender() {
if(status) {
    if(isProcessing) AudioDeviceStop(vDevice,process);
    OSStatus err = AudioDeviceRemoveIOProc(vDevice,process);
    if(err==noErr) status = false;
    for(int i =0;i< vChannels;i++) {
        free(inBuffers[i]);
    }
    for(int i =0;i< vChannels;i++) {
        free(outBuffers[i]);
    }
    free(inBuffers);
    free(outBuffers);
}
}

bool AudioRender::ConfigureAudioProc(float sampleRate,long bufferSize,int channels,int inChannels,char *device) {
    
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
    if(err!=noErr) return false;
    
    int manyDevices = size/sizeof(AudioDeviceID);
    
    AudioDeviceID devices[manyDevices];
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices);
    if(err!=noErr) return false;
    
    size = sizeof(AudioDeviceID);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,&size,&vDevice);
    if(err!=noErr) return false;
    
    for(int i=0;i<manyDevices;i++) {
        size = sizeof(char)*256;
        char name[256];
        err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name);
        if(err!=noErr) return false;
        if(strcmp(device,name)==0) vDevice = devices[i];
    }
    
    char deviceName[256];
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyDeviceName,&size,&deviceName);
    if(err!=noErr) return false;
    
    printf("DEVICE: %s\n",deviceName);
    
    /*size = sizeof(AudioBufferList);
    AudioBufferList bufList;
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamConfiguration,&size,&bufList);
    if(err!=noErr) return false;
    
    int nChan = 0;
    for(unsigned int i = 0;i<bufList.mNumberBuffers;i++) {
        nChan += (int)bufList.mBuffers[i].mNumberChannels;
    }*/
    
    
    size = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription SR;
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&SR);
    if(err!=noErr) return false;
    
    err = AudioDeviceGetPropertyInfo(vDevice,0,false,kAudioDevicePropertyStreams,&size,&isWritable);
    if(err!=noErr) return false;
    
    
    vChannels = (int)SR.mChannelsPerFrame*(size/sizeof(AudioStreamID));
    if(vChannels>=channels) vChannels = channels;
    
    printf("OUTPUT CHANNELS: %d\n",vChannels);
    
    err = AudioDeviceGetPropertyInfo(vDevice,0,true,kAudioDevicePropertyStreamFormat,&size,&isWritable);
    if(err!=noErr) { vInChannels = 0; goto endInChan; }
    
    size = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription inSR;
    err = AudioDeviceGetProperty(vDevice,0,true,kAudioDevicePropertyStreamFormat,&size,&inSR);
    if(err!=noErr) return false;
    
    err = AudioDeviceGetPropertyInfo(vDevice,0,true,kAudioDevicePropertyStreams,&size,&isWritable);
    if(err!=noErr) return false;
    
    
    vInChannels = (int)inSR.mChannelsPerFrame*(size/sizeof(AudioStreamID));

endInChan:

    if(vInChannels>=inChannels) vInChannels = inChannels;
    
    printf("INPUT CHANNELS: %d\n",vInChannels);
    
    UInt32 bufFrame;
    size = sizeof(UInt32);
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&bufFrame);
    if(err!=noErr) return false;
    
    //bufFrame *= SR.mChannelsPerFrame;
        
    vBufferSize = (long) bufFrame;
    
    if((long)bufFrame!=bufferSize) { 
        printf("I'm trying to set a new buffer size\n");
        UInt32 theSize = sizeof(UInt32);
        UInt32 newBufferSize = (UInt32) bufferSize;///SR.mChannelsPerFrame;
        err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufferSize);
        if(err!=noErr) printf("Cannot set a new buffer size\n");
        else {	
            UInt32 newBufFrame;
            size = sizeof(UInt32);
            err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&newBufFrame);
            if(err!=noErr) return false;
            vBufferSize = (long)newBufFrame;//*SR.mChannelsPerFrame;
        }
    }
    
    printf("BUFFER SIZE: %ld\n",vBufferSize);
    
    
    vSampleRate = (float)SR.mSampleRate;
    
    if((float)SR.mSampleRate!=sampleRate) {
        printf("I'm trying to set a new sample rate\n");
        UInt32 theSize = sizeof(AudioStreamBasicDescription);
        SR.mSampleRate = (Float64)sampleRate;
        err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyStreamFormat,theSize,&SR);
        if(err!=noErr) printf("Cannot set a new sample rate\n");
        else {
            size = sizeof(AudioStreamBasicDescription);
            AudioStreamBasicDescription newCheckSR;
            err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&newCheckSR);
            if(err!=noErr) return false;
            vSampleRate = (float)newCheckSR.mSampleRate;
        }
    }
    
    printf("SAMPLE RATE: %f\n",vSampleRate);
    
    err = AudioDeviceAddIOProc(vDevice,process,this);
    if(err!=noErr) return false;
    
    return true;
}

bool AudioRender::StartAudio() {
if(status) {
    OSStatus err = AudioDeviceStart(vDevice,process);
    if(err!=noErr) return false;
    AudioRender::isProcessing = true;
    return true;
} 
    return false;
}

bool AudioRender::StopAudio() {
if(status) {
    OSStatus err = AudioDeviceStop(vDevice,process);
    if(err!=noErr) return false;
    AudioRender::isProcessing = false;
    return true;
}
    return false;
}

OSStatus AudioRender::process(AudioDeviceID inDevice,const AudioTimeStamp* inNow,const AudioBufferList* inInputData,const AudioTimeStamp* 
                                    inInputTime,AudioBufferList* outOutputData,const AudioTimeStamp* inOutputTime,void* inClientData) 
{
    AudioRender *classe = (AudioRender*)inClientData;
    int channel = 0;
    
    AudioRender::gTime = inInputTime;
    
    for(int i=0;i<classe->vChannels;i++) {
        memset(classe->outBuffers[i],0x0,sizeof(float)*classe->vBufferSize); 
    }
    
if(inInputData->mNumberBuffers!=0 && inInputData->mBuffers[0].mNumberChannels==1) { //MONO MODE PROCESS
    long nframes = inInputData->mBuffers[0].mDataByteSize/sizeof(float);
    
    for(unsigned int a = 0; a < inInputData->mNumberBuffers;a++) {
        memcpy(classe->inBuffers[channel],inInputData->mBuffers[a].mData,inInputData->mBuffers[a].mDataByteSize);
        channel++;
        if(channel==classe->vInChannels) break;
    }
    
    //Jack RENDER
	classe->f_JackRunCycle(classe->jackData,classe->vBufferSize);
     
    goto output;
}

if(inInputData->mNumberBuffers!=0 && inInputData->mBuffers[0].mNumberChannels>1) { //NOT MONO MODE PROCESS
    long nFrames = (inInputData->mBuffers[0].mDataByteSize / sizeof(float))/inInputData->mBuffers[0].mNumberChannels;
    channel = 0;
    
    for(unsigned int b=0;b<inInputData->mNumberBuffers;b++) {
        for(unsigned int a=0; a<inInputData->mBuffers[b].mNumberChannels; a++) {
            for(long i=0;i<nFrames;i++) {
                float *inData = (float*)inInputData->mBuffers[b].mData;
                classe->inBuffers[channel][i] = inData[a+(i*2)];
            }
            channel++;
            if(channel==classe->vInChannels) break;
        }
        if(channel==classe->vInChannels) break;
    }
    
    //Jack RENDER
    classe->f_JackRunCycle(classe->jackData,classe->vBufferSize);
	
}

output:

if(outOutputData->mNumberBuffers!=0 && outOutputData->mBuffers[0].mNumberChannels==1) { //MONO MODE PROCESS
    channel = 0;
    
    for(unsigned int a = 0; a < outOutputData->mNumberBuffers;a++) {
        memcpy(outOutputData->mBuffers[a].mData,classe->outBuffers[channel],outOutputData->mBuffers[a].mDataByteSize);
        channel++;
        if(channel==classe->vChannels) break;
    }
    
    goto end;
}

if(outOutputData->mNumberBuffers!=0 && outOutputData->mBuffers[0].mNumberChannels>1) { //NOT MONO MODE PROCESS
    long nFrames = (outOutputData->mBuffers[0].mDataByteSize / sizeof(float))/outOutputData->mBuffers[0].mNumberChannels;
    
    channel = 0;
    
    for(unsigned int b=0;b<outOutputData->mNumberBuffers;b++) {
        for(unsigned int a=0; a<outOutputData->mBuffers[b].mNumberChannels; a++) {
            for(long i=0;i<nFrames;i++) {
                float *outData = (float*)outOutputData->mBuffers[b].mData;
                outData[a+(i*2)] = classe->outBuffers[channel][i];
            }
            channel++;
            if(channel==classe->vChannels) break;
        }
        if(channel==classe->vChannels) break;
    }

}

end:

    return noErr;
}

float **AudioRender::getADC() {
    if(AudioRender::theRender==NULL) return NULL;
    return AudioRender::theRender->inBuffers;
}
float **AudioRender::getDAC() {
    if(AudioRender::theRender==NULL) return NULL;
    return AudioRender::theRender->outBuffers;
}
