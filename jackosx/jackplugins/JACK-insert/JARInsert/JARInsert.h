/*
	JARInsert.h
	
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
 
#define DEBUG 1

#include <Jack/Jack.h>
#include <Carbon/Carbon.h>
#include <CoreAudio/CoreAudio.h>
#include "BSizeAlign.h"

#define SHOWALERT(err_str_) \
printf("JARInsert Critical Log: error = %d.\n",c_error);
/*
CFStringRef str = CFStringCreateWithCString(NULL,err_str_,NULL); \
CFUserNotificationDisplayNotice(0,kCFUserNotificationCautionAlertLevel,NULL,NULL,NULL,\
CFSTR("JACK-insert"),str,CFSTR("Ok"));
*/

enum {
    kAudioDevicePropertyGetJackClient  = 'jasg', kAudioDevicePropertyReleaseJackClient  = 'jasr'
};

class JARInsert {
	public:
		enum {
			kNoErr,kErrNoClient,kErrCoreAudio,kErrInvalidBSize
		};
		JARInsert(long host_buffer_size,int hostType);
		JARInsert(int hostType);
		~JARInsert();
		bool OpenAudioClient();
		void Flush();
		int GetError() { return c_error; }
		int Process(float **in_buffer,float **out_buffer,long host_nframes);
		bool AllocBSizeAlign(long host_buffer_size);
		bool CanProcess() { return c_canProcess; }
	private:
		bool c_canProcess;
		int c_error;
		AudioDeviceID c_jackDevID;
		jack_client_t *c_client;
		bool c_isRunning;
		bool c_rBufOn;
		bool c_needsDeactivate;
		int c_nInPorts,c_nOutPorts;
		jack_port_t **c_inPorts,**c_outPorts;
		long c_jBufferSize,c_hBufferSize;
		static int c_instances;
		BSizeAlign *c_bsAI1,*c_bsAI2,*c_bsAO1,*c_bsAO2;
};

