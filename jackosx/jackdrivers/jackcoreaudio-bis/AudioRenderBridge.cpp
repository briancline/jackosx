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

void *openPandaAudioInstance(float sampleRate,float virtual_sampleRate,long bufferSize,int inChannels, int outChannels,char *device) {
	AudioRender *newInst = new AudioRender(sampleRate,virtual_sampleRate,bufferSize,inChannels,outChannels,device);
	if(newInst->status) return newInst;
	return NULL;
}

void closePandaAudioInstance(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->StopAudio();
		delete inst;
	}
}

int startPandaAudioProcess(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->StartAudio();
	}
	return FALSE;
}

int stopPandaAudioProcess(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->StopAudio();
	}
	return FALSE;
}

float **getPandaAudioInputs(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->inBuffers;
	}
	return NULL;
}

float **getPandaAudioOutputs(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->outBuffers;
	}
	return NULL;
}

void * getHostData(void *instance) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		return inst->jackData;
	}
	return NULL;
}

void setHostData(void *instance, void* hostData) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->jackData = hostData;
	}
}

void setCycleFun(void *instance,JackRunCyclePtr fun) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		inst->f_JackRunCycle = fun;
	}
}

void setParameter(void *instance,int id,void *data) {
	if(instance) {
		AudioRender *inst = (AudioRender*)instance;
		switch(id) {
			case 'v_sr':
				inst->vir_SampleRate = *(float*)data;
				break;
		}
	}
}

