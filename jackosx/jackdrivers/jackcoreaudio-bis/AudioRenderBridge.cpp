/*
 *  AudioRenderBridge.c
 *  jack_coreaudio
 *  Under Artistic License.
 *
 *  Created by Johnny Petrantoni on Fri Jan 30 2004.
 *  Copyright (c) 2004 Johnny Petrantoni. All rights reserved.
 *
 */

#include "AudioRenderBridge.h"
#include "AudioRender.h"

void *openAudioInstance(float sampleRate,long bufferSize,int inChannels, int outChannels,char *device) {
	AudioRender *newInst = new AudioRender();
	bool res = newInst->ConfigureAudioProc(sampleRate,bufferSize,outChannels,inChannels,device);
	if(res) return newInst;
	return NULL;
}

void closeAudioInstance(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->StopAudio();
		delete inst;
	}
}

int startAudioProcess(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->StartAudio();
	}
	return FALSE;
}

int stopAudioProcess(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->StopAudio();
	}
	return FALSE;
}

void * getHostData(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->GetHostData();
	}
	return NULL;
}

void setHostData(void *instance, void* hostData) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->SetHostData(hostData);
	}
}

void setCycleFun(void *instance,RenderCyclePtr fun) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->SetRenderCallback(fun);
	}
}

void setParameter(void *instance,int id,void *data) {
	if(instance) {
		//AudioRender *inst = (AudioRender*)instance;
	}
}

