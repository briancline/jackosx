/*
  Copyright ©  Johnny Petrantoni 2003
 
  This library is free software; you can redistribute it and modify it under
  the terms of the GNU Library General Public License as published by the
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// JackVST.cpp

#include <JackVST.hpp>

int JackVST::instances = 0;
//-------------------------------------------------------------------------------------------------------
JackVST::JackVST (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, 1)	// 1 program, 1 parameter only
{
	fGain = 1.;				// default to 0 dB
	setNumInputs (2);		// stereo in
	setNumOutputs (2);		// stereo out
	setUniqueID ('JACK-insert');	// identify
	canMono ();				// makes sense to feed both inputs with the same signal
	canProcessReplacing ();	// supports both accumulating and replacing output
	strcpy (programName, "Default");	// default program name
	status = 2;
        
	//JASBus INIT
    printf("-----------JASBus-----------log-----\n");
    printf("There are %d istances of JASBus\n",instances);
    
    jackIsOn = false;

    openAudioFTh();
    if(!jackIsOn) printf("CLIENT NULL\n");
    else {
           
        int nPorte=2;
    
        inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
        outPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
    
    
        jBufferSize = jack_get_buffer_size (client);
    
        nInPorts = nOutPorts =  nPorte;
    
    
        for(int i=0;i<nInPorts;i++) {
            char *newName = (char*)calloc(256,sizeof(char));
            sprintf(newName,"VSTreturn%d",instances+i+1);
            inPorts[i] = jack_port_register(client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
            printf("Port: %s created\n",newName);
            free(newName);
        }
    
        for(int i=0;i<nOutPorts;i++) {
            char *newName = (char*)calloc(256,sizeof(char));
            sprintf(newName,"VSTsend%d",instances+i+1);
            outPorts[i] = jack_port_register(client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
            printf("Port: %s created\n",newName);
            free(newName);
        }
        
        if(!isRunning) { printf("Jack client activated\n"); jack_activate(client); needsDeactivate = true; }
        else needsDeactivate = false;
        
        status = 77;
        instances += 2;
        
        rBufOn = false;
    } 

}

//-------------------------------------------------------------------------------------------------------
JackVST::~JackVST ()
{
	if(status==77) {
            status==2;
            flush();
        }
}

//-------------------------------------------------------------------------------------------------------
void JackVST::setProgramName (char *name)
{
	strcpy (programName, name);
}

//-----------------------------------------------------------------------------------------
void JackVST::getProgramName (char *name)
{
	strcpy (name, programName);
}

//-----------------------------------------------------------------------------------------
void JackVST::setParameter (long index, float value)
{
	fGain = value;
}

//-----------------------------------------------------------------------------------------
float JackVST::getParameter (long index)
{
	return fGain;
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterName (long index, char *label)
{
    if(jackIsOn)
	strcpy (label, "ONLINE");
    else 
        strcpy (label, "OFFLINE");
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterDisplay (long index, char *text)
{
	dB2string (fGain, text);
}

//-----------------------------------------------------------------------------------------
void JackVST::getParameterLabel(long index, char *label)
{
	strcpy (label, "");
}

//------------------------------------------------------------------------
bool JackVST::getEffectName (char* name)
{
	strcpy (name, "JACK-insert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getProductString (char* text)
{
	strcpy (text, "JACK-insert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getVendorString (char* text)
{
	strcpy (text, "(c) 2003, Johnny Petrantoni.");
	return true;
}

//-----------------------------------------------------------------------------------------
void JackVST::process (float **inputs, float **outputs, long sampleFrames)
{
    
    float *pfInBuffer0  =  inputs[0];
    float *pfInBuffer1  =  inputs[1];
    float *pfOutBuffer0 = outputs[0];
    float *pfOutBuffer1 = outputs[1];
    
    if(status==77) {
    
    if(jBufferSize>sampleFrames && !rBufOn) { 
        printf("Setting up a ring buffer\nJack buffer size: %ld , host buffer size: %ld\n",jBufferSize,sampleFrames); 
        
        vRBuffer1 = (float*)malloc(sizeof(float)*jBufferSize*2);
        sRingBuffer1 = (RingBuffer*)malloc(sizeof(RingBuffer));
        long size = RingBuffer_Init(sRingBuffer1,sizeof(float)*jBufferSize*2,vRBuffer1);
        
        manyInBuffers = (jBufferSize/sampleFrames)*2;
        
        inFromRing = (float**)malloc(sizeof(float*)*manyInBuffers);
        for(int i=0;i<manyInBuffers;i++) {
            inFromRing[i] = (float*)malloc(sizeof(float)*sampleFrames);
        }
        
        
        if(size==-1) { printf("Cannot create a correct ring buffer\n"); return; }
        
        vRBuffer2 = (float*)malloc(sizeof(float)*jBufferSize*2);
        sRingBuffer2 = (RingBuffer*)malloc(sizeof(RingBuffer));
        size = RingBuffer_Init(sRingBuffer2,sizeof(float)*jBufferSize*2,vRBuffer2);
        outFromRing = (float*)malloc(sizeof(float)*sampleFrames);
        
        if(size==-1) { printf("Cannot create a correct ring buffer\n"); return; }
        
        printf("ring buffer created\n");
        
        rBufOn = true;
    }
    
    if(jBufferSize>sampleFrames && rBufOn) {
                
        if(RingBuffer_GetWriteAvailable(sRingBuffer1)>=(sizeof(float)*sampleFrames*2)) {
            RingBuffer_Write(sRingBuffer1,pfInBuffer0,sizeof(float)*sampleFrames);
            RingBuffer_Write(sRingBuffer1,pfInBuffer1,sizeof(float)*sampleFrames);
        }
                
        if(RingBuffer_GetReadAvailable(sRingBuffer1)==sizeof(float)*jBufferSize*2) {
            float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)jBufferSize);
            float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)jBufferSize);
            
            int manyLoops = manyInBuffers/2;
            
            int a = 0;
            
            for(int i=0;i<manyLoops;i++) {
                RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*sampleFrames);
                for(int l=0;l<sampleFrames;l++) {
                    *out1 = inFromRing[a][l];
                    *out1++;
                }
                a++;
                RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*sampleFrames);
                for(int l=0;l<sampleFrames;l++) {
                    *out2 = inFromRing[a][l];
                    *out2++; 
                }
                a++;
            }
        }
        
        if(RingBuffer_GetWriteAvailable(sRingBuffer2)==(sizeof(float)*jBufferSize*2)) {
            float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)jBufferSize);
            float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)jBufferSize);
            
            int manyLoops = manyInBuffers/2;
            
            int a = 0;
            
            for(int i=0;i<manyLoops;i++) {
                for(int l=0;l<sampleFrames;l++) {
                    outFromRing[l] = *in1;
                    *in1++;
                }
                RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*sampleFrames);
                a++;
                for(int l=0;l<sampleFrames;l++) {
                    outFromRing[l] = *in2;
                    *in2++;
                }
                RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*sampleFrames);
                a++;
            }
        }
        
        if(RingBuffer_GetReadAvailable(sRingBuffer2)>=(sizeof(float)*sampleFrames*2)) {
            RingBuffer_Read(sRingBuffer2,pfOutBuffer0,sizeof(float)*sampleFrames);
            RingBuffer_Read(sRingBuffer2,pfOutBuffer1,sizeof(float)*sampleFrames);
        }
        
    }
    
     if(jBufferSize==sampleFrames) {
        float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)sampleFrames);
        float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)sampleFrames);
    
        float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)sampleFrames);
        float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)sampleFrames);
    
        memcpy(pfOutBuffer0,in1,sizeof(float)*sampleFrames);
        memcpy(pfOutBuffer1,in2,sizeof(float)*sampleFrames);
        memcpy(out1,pfInBuffer0,sizeof(float)*sampleFrames);
        memcpy(out2,pfInBuffer1,sizeof(float)*sampleFrames);
    }
    
    }

}

//-----------------------------------------------------------------------------------------
void JackVST::processReplacing (float **inputs, float **outputs, long sampleFrames)
{

    float *pfInBuffer0  =  inputs[0];
    float *pfInBuffer1  =  inputs[1];
    float *pfOutBuffer0 = outputs[0];
    float *pfOutBuffer1 = outputs[1];
    
    if(status==77) {
    
    if(jBufferSize>sampleFrames && !rBufOn) { 
        printf("Setting up a ring buffer\nJack buffer size: %ld , host buffer size: %ld\n",jBufferSize,sampleFrames); 
        
        vRBuffer1 = (float*)malloc(sizeof(float)*jBufferSize*2);
        sRingBuffer1 = (RingBuffer*)malloc(sizeof(RingBuffer));
        long size = RingBuffer_Init(sRingBuffer1,sizeof(float)*jBufferSize*2,vRBuffer1);
        
        manyInBuffers = (jBufferSize/sampleFrames)*2;
        
        inFromRing = (float**)malloc(sizeof(float*)*manyInBuffers);
        for(int i=0;i<manyInBuffers;i++) {
            inFromRing[i] = (float*)malloc(sizeof(float)*sampleFrames);
        }
        
        
        if(size==-1) { printf("Cannot create a correct ring buffer\n"); return; }
        
        vRBuffer2 = (float*)malloc(sizeof(float)*jBufferSize*2);
        sRingBuffer2 = (RingBuffer*)malloc(sizeof(RingBuffer));
        size = RingBuffer_Init(sRingBuffer2,sizeof(float)*jBufferSize*2,vRBuffer2);
        outFromRing = (float*)malloc(sizeof(float)*sampleFrames);
        
        if(size==-1) { printf("Cannot create a correct ring buffer\n"); return; }
        
        printf("ring buffer created\n");
        
        rBufOn = true;
    }
    
    if(jBufferSize>sampleFrames && rBufOn) {
                
        if(RingBuffer_GetWriteAvailable(sRingBuffer1)>=(sizeof(float)*sampleFrames*2)) {
            RingBuffer_Write(sRingBuffer1,pfInBuffer0,sizeof(float)*sampleFrames);
            RingBuffer_Write(sRingBuffer1,pfInBuffer1,sizeof(float)*sampleFrames);
        }
                
        if(RingBuffer_GetReadAvailable(sRingBuffer1)==sizeof(float)*jBufferSize*2) {
            float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)jBufferSize);
            float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)jBufferSize);
            
            int manyLoops = manyInBuffers/2;
            
            int a = 0;
            
            for(int i=0;i<manyLoops;i++) {
                RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*sampleFrames);
                for(int l=0;l<sampleFrames;l++) {
                    *out1 = inFromRing[a][l];
                    *out1++;
                }
                a++;
                RingBuffer_Read(sRingBuffer1,inFromRing[a],sizeof(float)*sampleFrames);
                for(int l=0;l<sampleFrames;l++) {
                    *out2 = inFromRing[a][l];
                    *out2++; 
                }
                a++;
            }
        }
        
        if(RingBuffer_GetWriteAvailable(sRingBuffer2)==(sizeof(float)*jBufferSize*2)) {
            float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)jBufferSize);
            float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)jBufferSize);
            
            int manyLoops = manyInBuffers/2;
            
            int a = 0;
            
            for(int i=0;i<manyLoops;i++) {
                for(int l=0;l<sampleFrames;l++) {
                    outFromRing[l] = *in1;
                    *in1++;
                }
                RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*sampleFrames);
                a++;
                for(int l=0;l<sampleFrames;l++) {
                    outFromRing[l] = *in2;
                    *in2++;
                }
                RingBuffer_Write(sRingBuffer2,outFromRing,sizeof(float)*sampleFrames);
                a++;
            }
        }
        
        if(RingBuffer_GetReadAvailable(sRingBuffer2)>=(sizeof(float)*sampleFrames*2)) {
            RingBuffer_Read(sRingBuffer2,pfOutBuffer0,sizeof(float)*sampleFrames);
            RingBuffer_Read(sRingBuffer2,pfOutBuffer1,sizeof(float)*sampleFrames);
        }
        
    }
    
     if(jBufferSize==sampleFrames) {
        float *in1 = (float*) jack_port_get_buffer(inPorts[0],(jack_nframes_t)sampleFrames);
        float *in2 = (float*) jack_port_get_buffer(inPorts[1],(jack_nframes_t)sampleFrames);
    
        float *out1 = (float*) jack_port_get_buffer(outPorts[0],(jack_nframes_t)sampleFrames);
        float *out2 = (float*) jack_port_get_buffer(outPorts[1],(jack_nframes_t)sampleFrames);
    
        memcpy(pfOutBuffer0,in1,sizeof(float)*sampleFrames);
        memcpy(pfOutBuffer1,in2,sizeof(float)*sampleFrames);
        memcpy(out1,pfInBuffer0,sizeof(float)*sampleFrames);
        memcpy(out2,pfInBuffer1,sizeof(float)*sampleFrames);
    }
    
    }
    
}

void JackVST::openAudioFTh() {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    AudioDeviceID jackDevID = 9999;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
    
    int nDevices = size/sizeof(AudioDeviceID); //here I'm counting how many devices are present
    printf("There is %d audio devices\n",nDevices);
    AudioDeviceID *device = (AudioDeviceID*)calloc(nDevices,sizeof(AudioDeviceID));
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,device);
    
    for(int i=0;i<nDevices;i++) {
        printf("ID: %ld\n",device[i]);
        char name[256];
        size = sizeof(char)*256;
        AudioDeviceGetProperty(device[i],0,true,kAudioDevicePropertyDeviceName,&size,&name);
        printf("Name: %s\n",name);
        if(strcmp(&name[0],"Jack Audio Server")==0) { jackDevID=device[i]; jackIsOn = true; if(device!=NULL) free(device); device=NULL; break; }
    }
    
    if(device!=NULL) free(device);
    
    if(jackDevID==9999) return;
    jackID = jackDevID;
    err = AudioDeviceGetProperty(jackDevID,0,true,kAudioDevicePropertyGetJackClient,&size,&client);
    
    err = AudioDeviceGetProperty(jackDevID,0,true,kAudioDevicePropertyDeviceIsRunning,&size,&isRunning);
        
    if(client!=NULL) printf("NON NULLO\n");
    else { client = NULL; jackIsOn = false; }
    
}

void JackVST::flush() {
    if(client!=NULL) {
        
        if(rBufOn) {
            printf("Flushing ring buffer 1\n");
            free(vRBuffer1);
            free(vRBuffer2);
            free(outFromRing);
            for(int i = 0;i<manyInBuffers;i++) {
                free(inFromRing[i]);
            }
            free(inFromRing);
            RingBuffer_Flush(sRingBuffer1);
            RingBuffer_Flush(sRingBuffer2);
            free(sRingBuffer1);
            free(sRingBuffer2);
        }
        
        if(needsDeactivate) { printf("Needs Deactivate client\n"); jack_deactivate(client); }

        for(int i=0;i<nInPorts;i++) {
            jack_port_unregister(client,inPorts[i]);
        }
        free(inPorts);
        for(int i=0;i<nOutPorts;i++) {
            jack_port_unregister(client,outPorts[i]);
        }
        
        UInt32 size;
        OSStatus err = AudioDeviceGetProperty(jackID,0,true,kAudioDevicePropertyReleaseJackClient,&size,&client);
        
        free(outPorts);
        
    }
}

