/*
	JARInsert.cpp
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.


	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	Lesser General Public License for more details.


	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
	
	(c) 2004, elementicaotici - by Johnny (Giovanni) Petrantoni, ITALY - Rome.
	e-mail: johnny@meskalina.it web: http://www.meskalina.it 
*/

#include "JARInsert.h"

extern "C" void JARILog(char *fmt,...) {
#ifdef DEBUG
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"JARInsert Log: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

int JARInsert::c_instances = 0;

JARInsert::JARInsert(long host_buffer_size) 
	: c_error(0),c_client(NULL),c_isRunning(false),c_rBufOn(false),c_needsDeactivate(false),c_hBufferSize(host_buffer_size)
{	
	if(!OpenAudioClient()) { 
		JARILog("Cannot find jack client.\n");
		SHOWALERT("Cannot find jack client for this application, check if Jack server is running.");
		return; 
	}
		
	int nPorts = 2;
        
	c_inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorts);
	c_outPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorts);
        
	c_nInPorts = c_nOutPorts =  nPorts;
            
	c_jBufferSize = jack_get_buffer_size(c_client);
	
	char *newName = (char*)calloc(256,sizeof(char));
        
	for(int i=0;i<c_nInPorts;i++) {
		sprintf(newName,"AUreturn%d",JARInsert::c_instances+i+1);
		c_inPorts[i] = jack_port_register(c_client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
		JARILog("Port: %s created\n",newName);
	}
        
	for(int i=0;i<c_nOutPorts;i++) {
		sprintf(newName,"AUsend%d",JARInsert::c_instances+i+1);
		c_outPorts[i] = jack_port_register(c_client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
		JARILog("Port: %s created\n",newName);
	}
	
	free(newName);
			
	float *out1 = (float*) jack_port_get_buffer(c_outPorts[0],(jack_nframes_t)c_jBufferSize);
	float *out2 = (float*) jack_port_get_buffer(c_outPorts[1],(jack_nframes_t)c_jBufferSize);
			
	if(out1) memset(out1,0x0,sizeof(float)*c_jBufferSize);
	if(out2) memset(out2,0x0,sizeof(float)*c_jBufferSize);
	
	#if 1
	if(!c_isRunning) { 
		JARILog("Jack client activated\n"); 
		jack_activate(c_client); 
		c_needsDeactivate = true;
	} else c_needsDeactivate = false;
	#endif
		
	JARInsert::c_instances += 2;
            
	if(c_jBufferSize>c_hBufferSize) {
		c_bsAI1 = new BSizeAlign(c_hBufferSize,c_jBufferSize);
		c_bsAI2 = new BSizeAlign(c_hBufferSize,c_jBufferSize);
		c_bsAO1 = new BSizeAlign(c_jBufferSize,c_hBufferSize);
		c_bsAO2 = new BSizeAlign(c_jBufferSize,c_hBufferSize);
		if(c_bsAI1->Ready() && c_bsAI2->Ready() && c_bsAO1->Ready() && c_bsAO2->Ready()) c_rBufOn = true;
		else { c_error = -94; Flush(); return; }
	}
	
	c_canProcess = true;
}

JARInsert::JARInsert() 
	: c_error(0),c_client(NULL),c_isRunning(false),c_rBufOn(false),c_needsDeactivate(false),c_hBufferSize(0)
{
	if(!OpenAudioClient()) { 
		JARILog("Cannot find jack client.\n");
		SHOWALERT("Cannot find jack client for this application, check if Jack server is running.");
		return; 
	}
		
	int nPorts = 2;
        
	c_inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorts);
	c_outPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorts);
        
	c_nInPorts = c_nOutPorts =  nPorts;
            
	c_jBufferSize = jack_get_buffer_size(c_client);
	
	char *newName = (char*)calloc(256,sizeof(char));
        
	for(int i=0;i<c_nInPorts;i++) {
		sprintf(newName,"AUreturn%d",JARInsert::c_instances+i+1);
		c_inPorts[i] = jack_port_register(c_client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
		JARILog("Port: %s created\n",newName);
	}
        
	for(int i=0;i<c_nOutPorts;i++) {
		sprintf(newName,"AUsend%d",JARInsert::c_instances+i+1);
		c_outPorts[i] = jack_port_register(c_client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
		JARILog("Port: %s created\n",newName);
	}
	
	free(newName);
			
	float *out1 = (float*) jack_port_get_buffer(c_outPorts[0],(jack_nframes_t)c_jBufferSize);
	float *out2 = (float*) jack_port_get_buffer(c_outPorts[1],(jack_nframes_t)c_jBufferSize);
			
	if(out1) memset(out1,0x0,sizeof(float)*c_jBufferSize);
	if(out2) memset(out2,0x0,sizeof(float)*c_jBufferSize);
	
	#if 1
	if(!c_isRunning) { 
		JARILog("Jack client activated\n"); 
		jack_activate(c_client); 
		c_needsDeactivate = true;
	} else c_needsDeactivate = false;
    #endif    
	
	JARInsert::c_instances += 2;
	
	c_canProcess = false;
}

JARInsert::~JARInsert() {
	if(c_error == 0) Flush();
}

bool JARInsert::AllocBSizeAlign(long host_buffer_size) {
	c_hBufferSize = host_buffer_size;
	if(c_jBufferSize>c_hBufferSize) {
		c_bsAI1 = new BSizeAlign(c_hBufferSize,c_jBufferSize);
		c_bsAI2 = new BSizeAlign(c_hBufferSize,c_jBufferSize);
		c_bsAO1 = new BSizeAlign(c_jBufferSize,c_hBufferSize);
		c_bsAO2 = new BSizeAlign(c_jBufferSize,c_hBufferSize);
		if(c_bsAI1->Ready() && c_bsAI2->Ready() && c_bsAO1->Ready() && c_bsAO2->Ready()) c_rBufOn = true;
		else { c_error = -165; Flush(); return false; }
	}
	c_canProcess = true;
	if(c_rBufOn) JARILog("Using BSizeAlign.\n");
	else JARILog("Not Using BSizeAlign.\n");
	return true;
}

int JARInsert::Process(float **in_buffer,float **out_buffer) {
	if(c_rBufOn) {
		float *out1 = (float*) jack_port_get_buffer(c_outPorts[0],(jack_nframes_t)c_jBufferSize);
		float *out2 = (float*) jack_port_get_buffer(c_outPorts[1],(jack_nframes_t)c_jBufferSize);
		float *in1 = (float*) jack_port_get_buffer(c_inPorts[0],(jack_nframes_t)c_jBufferSize);
		float *in2 = (float*) jack_port_get_buffer(c_inPorts[1],(jack_nframes_t)c_jBufferSize);
			
		c_bsAI1->AddBuffer(in_buffer[0]);
		c_bsAI2->AddBuffer(in_buffer[1]);
			
		if(!c_bsAO1->CanGet()) c_bsAO1->AddBuffer(in1);
		if(!c_bsAO2->CanGet()) c_bsAO2->AddBuffer(in2);
			
		if(c_bsAO1->CanGet()) c_bsAO1->GetBuffer(out_buffer[0]);
		if(c_bsAO2->CanGet()) c_bsAO2->GetBuffer(out_buffer[1]);
			
		if(c_bsAI1->CanGet()) c_bsAI1->GetBuffer(out1);
		if(c_bsAI2->CanGet()) c_bsAI2->GetBuffer(out2);
	} else {
		float *out1 = (float*) jack_port_get_buffer(c_outPorts[0],(jack_nframes_t)c_jBufferSize);
		float *out2 = (float*) jack_port_get_buffer(c_outPorts[1],(jack_nframes_t)c_jBufferSize);
		float *in1 = (float*) jack_port_get_buffer(c_inPorts[0],(jack_nframes_t)c_jBufferSize);
		float *in2 = (float*) jack_port_get_buffer(c_inPorts[1],(jack_nframes_t)c_jBufferSize);
		
		memcpy(out1,in_buffer[0],sizeof(float)*c_jBufferSize);
		memcpy(out2,in_buffer[1],sizeof(float)*c_jBufferSize);
		memcpy(out_buffer[0],in1,sizeof(float)*c_jBufferSize);
		memcpy(out_buffer[1],in2,sizeof(float)*c_jBufferSize);
	}
	return 0;
}

bool JARInsert::OpenAudioClient() {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
	if(err!=noErr) { c_error = -211; return false; }
    
    int nDevices = size/sizeof(AudioDeviceID);
	
    JARILog("There are %d audio devices\n",nDevices);

    AudioDeviceID *device = (AudioDeviceID*)calloc(nDevices,sizeof(AudioDeviceID));
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,device);
	if(err!=noErr) { c_error = -219; return false; }
    
    for(int i=0;i<nDevices;i++) {
	
        JARILog("ID: %ld\n",device[i]);

        char name[256];
        size = sizeof(char)*256;
        err = AudioDeviceGetProperty(device[i],0,true,kAudioDevicePropertyDeviceName,&size,&name);
		if(err==noErr) { 
			JARILog("Name: %s\n",name);

			if(strcmp(&name[0],"Jack Audio Server")==0 || strcmp(&name[0],"Jack Router")==0) { 
				c_jackDevID=device[i]; 
				if(device!=NULL) free(device); 
				err = AudioDeviceGetProperty(c_jackDevID,0,true,kAudioDevicePropertyGetJackClient,&size,&c_client);
				if(err!=noErr) { c_error = -235; return false; }
				#if 1
				err = AudioDeviceGetProperty(c_jackDevID,0,true,kAudioDevicePropertyDeviceIsRunning,&size,&c_isRunning);
				if(err!=noErr) { c_error = -238; return false; }
				#endif
				if(c_client!=NULL) { c_error = 0; return true; }
				else return false;
			}
		}
    }
	c_error = -245;
	return false;
}

void JARInsert::Flush() {
    JARILog("Running flush\n");
    if(c_client!=NULL) {
        if(c_rBufOn) {
			delete c_bsAO1;
			delete c_bsAO2;
			delete c_bsAI1;
			delete c_bsAI2;
        }
		
		#if 1
        if(c_needsDeactivate) { 
			JARILog("Needs Deactivate client\n");
			jack_deactivate(c_client); 
		}
		#endif
		
        for(int i=0;i<c_nInPorts;i++) {
            jack_port_unregister(c_client,c_inPorts[i]);
        }
        free(c_inPorts);
        for(int i=0;i<c_nOutPorts;i++) {
            jack_port_unregister(c_client,c_outPorts[i]);
        }
        free(c_outPorts);        
		UInt32 size;
		AudioDeviceGetProperty(c_jackDevID,0,true,kAudioDevicePropertyReleaseJackClient,&size,&c_client);
    }
}
