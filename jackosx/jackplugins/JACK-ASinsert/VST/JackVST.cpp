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
jack_client_t *JackVST::client = NULL;
list<JackVST*> JackVST::classInstances;

static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	cond1 = PTHREAD_COND_INITIALIZER;
static bool isFlushing = FALSE;
//-------------------------------------------------------------------------------------------------------
JackVST::JackVST (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, 1)	// 1 program, 1 parameter only
{
	fGain = 1.;				// default to 0 dB
	setNumInputs (2);		// stereo in
	setNumOutputs (2);		// stereo out
	setUniqueID ('JACK-ASinsert');	// identify
	canMono ();				// makes sense to feed both inputs with the same signal
	canProcessReplacing ();	// supports both accumulating and replacing output
	strcpy (programName, "Default");	// default program name
	status = kIsOff;
        

    
    jackIsOn = false;
	status = kIsOff;
	
	jack_client_t *jack_test_client = jack_client_new("JACK-ASinsert-test");
	if(jack_test_client) { jackIsOn = true; jack_client_close(jack_test_client); }
	
    if(!jackIsOn) printf("Jack server is not running.\n");
    else {
		if(!JackVST::client) {
			JackVST::client = jack_client_new("JACK-ASinsert");
			jack_set_process_callback(JackVST::client,jackProcess,NULL);
		}
		
        int nPorte=MAX_PORTS;
    
        inPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
        outPorts = (jack_port_t**)malloc(sizeof(jack_port_t*)*nPorte);
    
    
        jBufferSize = jack_get_buffer_size (client);
    
        nInPorts = nOutPorts =  nPorte;
    
        for(int i=0;i<nInPorts;i++) {
            char *newName = (char*)calloc(256,sizeof(char));
            sprintf(newName,"VSTreturn%d",instances+i+1);
            inPorts[i] = jack_port_register(JackVST::client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
            printf("Port: %s created\n",newName);
            free(newName);
        }
    
        for(int i=0;i<nOutPorts;i++) {
            char *newName = (char*)calloc(256,sizeof(char));
            sprintf(newName,"VSTsend%d",instances+i+1);
            outPorts[i] = jack_port_register(JackVST::client,newName,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
            printf("Port: %s created\n",newName);
            free(newName);
        }
		
		for(int i=0;i<2;i++) {
			vRBufferIn[i] = (float*)malloc(sizeof(float)*jBufferSize*32);
			if(RingBuffer_Init(&sRingBufferIn[i],jBufferSize*32*sizeof(float),vRBufferIn[i])==-1) printf("error while creating ring buffer.\n");
		}
		
		for(int i=0;i<2;i++) {
			vRBufferOut[i] = (float*)malloc(sizeof(float)*jBufferSize*32);
			if(RingBuffer_Init(&sRingBufferOut[i],jBufferSize*32*sizeof(float),vRBufferOut[i])==-1) printf("error while creating ring buffer.\n");
		}
        
        jack_activate(JackVST::client); 
        
        status = kIsOn;
        instances += MAX_PORTS;
		
		ID = rand();
		
		JackVST::classInstances.push_front(this);
		
    } 

}

//-------------------------------------------------------------------------------------------------------
JackVST::~JackVST ()
{
	if(status==kIsOn) {
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
	strcpy (name, "JACK-ASinsert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getProductString (char* text)
{
	strcpy (text, "JACK-ASinsert");
	return true;
}

//------------------------------------------------------------------------
bool JackVST::getVendorString (char* text)
{
	strcpy (text, "(c) 2004, Johnny Petrantoni.");
	return true;
}

//-----------------------------------------------------------------------------------------

int JackVST::jackProcess(jack_nframes_t nframes, void *arg) {
	
	list<JackVST*>::iterator it;
	
	if(isFlushing) { pthread_cond_signal(&cond1); return 0; }
	
	for(it = JackVST::classInstances.begin(); it != JackVST::classInstances.end(); ++it) {
		if(*it) {
			JackVST *c = *it;
			if(!c) break;
			if(c->status == kIsOff) break;
			float *inBuffers[MAX_PORTS];
			for(int i=0;i<c->nInPorts;i++) {
				inBuffers[i] = (float*) jack_port_get_buffer(c->inPorts[i],nframes);
				RingBuffer_Write(&c->sRingBufferIn[i],inBuffers[i],nframes*sizeof(float));
			}
			float *outBuffers[MAX_PORTS];
			for(int i=0;i<c->nOutPorts;i++) {
				outBuffers[i] = (float*) jack_port_get_buffer(c->outPorts[i],nframes);
				RingBuffer_Read(&c->sRingBufferOut[i],outBuffers[i],nframes*sizeof(float));
			}
		}
	}
	
	return 0;
}

void JackVST::process (float **inputs, float **outputs, long sampleFrames)
{
	if(status==kIsOn) {
		for(int i=0;i<nOutPorts;i++) {
			RingBuffer_Write(&sRingBufferOut[i],inputs[i],sizeof(float)*sampleFrames);
		}
		for(int i=0;i<nInPorts;i++) {
			RingBuffer_Read(&sRingBufferIn[i],outputs[i],sizeof(float)*sampleFrames);
		}
		return;
    }
	
	
	return;
}

//-----------------------------------------------------------------------------------------
void JackVST::processReplacing (float **inputs, float **outputs, long sampleFrames)
{	
    if(status==kIsOn) {
		for(int i=0;i<nOutPorts;i++) {
			RingBuffer_Write(&sRingBufferOut[i],inputs[i],sizeof(float)*sampleFrames);
		}
		for(int i=0;i<nInPorts;i++) {
			RingBuffer_Read(&sRingBufferIn[i],outputs[i],sizeof(float)*sampleFrames);
		}
		return;
    }
	
	return;    
}

void JackVST::flush() {
	
	status = kIsOff;
	isFlushing = TRUE;
	
	list<JackVST*>::iterator it;
	
	printf("actually there are %ld instances.\n",JackVST::classInstances.size());
	
	pthread_cond_wait(&cond1, &mutex);
	JackVST::classInstances.remove(this);
	
	printf("now there are %ld instances.\n",JackVST::classInstances.size());
    
	RingBuffer_Flush(&sRingBufferIn[0]);
	RingBuffer_Flush(&sRingBufferIn[1]);
	RingBuffer_Flush(&sRingBufferOut[0]);
	RingBuffer_Flush(&sRingBufferOut[1]);    
	
	free(vRBufferIn[0]);
	free(vRBufferIn[1]);
	free(vRBufferOut[0]);
	free(vRBufferOut[1]);
	            
	for(int i=0;i<nInPorts;i++) {
		jack_port_unregister(JackVST::client,inPorts[i]);
		printf("unregistering in port %d.\n",i);
	}
	free(inPorts);
	for(int i=0;i<nOutPorts;i++) {
		jack_port_unregister(JackVST::client,outPorts[i]);
		printf("unregistering out port %d.\n",i);
	}
	free(outPorts);
				
	if(!JackVST::classInstances.size()) { 
		printf("closing client.\n"); 
		jack_deactivate(JackVST::client); 
		jack_client_close(JackVST::client);
		JackVST::client = NULL;
		JackVST::instances = 0;
	} else printf("client won't be closed.\n"); 
	
	isFlushing = FALSE;

}

