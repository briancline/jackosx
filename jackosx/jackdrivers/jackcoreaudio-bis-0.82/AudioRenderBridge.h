/*
 *  AudioRenderBridge.h
 *  jack_coreaudio
 *  Under Artistic License.
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

typedef int (*RenderCyclePtr)(void *data,long nframes,float **inBuffers,float **outBuffers);

#ifdef __cplusplus
extern "C" {
#endif

void *openAudioInstance(float sampleRate,long bufferSize,int inChannels,int outChannels,char *device);
void closeAudioInstance(void *instance);
int startAudioProcess(void *instance);
int stopAudioProcess(void *instance);
void * getHostData(void *instance);
void setHostData(void *instance, void* hostData);
void setCycleFun(void *instance,RenderCyclePtr fun);
void setParameter(void *instance,int id,void *data);

#ifdef __cplusplus
}
#endif

