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

// JackVST.hpp

#include "audioeffectx.h"
#include <CoreAudio/CoreAudio.h>
#include <Jack/jack.h>
#include <stdio.h>
#include <memory.h>
#include <ringbuffer.h>
#include <Carbon/Carbon.h>


//-------------------------------------------------------------------------------------------------------

enum {
    kAudioDevicePropertyGetJackClient  = 'jasg', kAudioDevicePropertyReleaseJackClient  = 'jasr'
};


class GuiComm {
    public:
        char fileName[256];
        char * getFileName() { return fileName; }
        void setFileName(char *name) { strcpy(fileName,name); }
};



class JackVST : public AudioEffectX
{
public:
	JackVST (audioMasterCallback audioMaster);
	~JackVST ();
	// Processes
        GuiComm laGui;
	virtual void process (float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing (float **inputs, float **outputs, long sampleFrames);

	// Program
	virtual void setProgramName (char *name);
	virtual void getProgramName (char *name);

	// Parameters
	virtual void setParameter (long index, float value);
	virtual float getParameter (long index);
	virtual void getParameterLabel (long index, char *label);
	virtual void getParameterDisplay (long index, char *text);
	virtual void getParameterName (long index, char *text);

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual long getVendorVersion () { return 1000; }
	
	virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }
        static int instances;
        
        
private:
        UInt32 isRunning;
        bool needsDeactivate;
        int manyInBuffers;
        bool rBufOn;
        float *vRBuffer1;
        float *vRBuffer2;
        float **inFromRing;
        float *outFromRing;
        RingBuffer *sRingBuffer1;
        RingBuffer *sRingBuffer2;
        long jBufferSize;
        AudioDeviceID jackID;
        
        jack_client_t *client;
        jack_port_t **inPorts;
        jack_port_t **outPorts;
        int nInPorts,nOutPorts;
        int conto;
        bool jackIsOn;
        void openAudioFTh();
	float fGain;
	char programName[32];
        void flush();
        int status;

};


