/*
 * Copyright (C) 2006 Robert Reif
 * Copyright (C) 2007 Peter L Jones
 * 
 * Copyright (C) 2007 Johnny Petrantoni
 * Copyright (C) 2007 Stephane Letz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
Changes Log:
28-9-2007: Johnny Petrantoni -	First Import.
28-9-2007: Johnny Petrantoni -	Added Stephane GetEXEName() and code cleanup.
28-9-2007: Johnny Petrantoni -	Minor fixes, added TODO and this changes log.
28-9-2007: Johnny Petrantoni -	Added pThreadUtilities.h code... but the cracks are still there..
29-9-2007: Johnny Petrantoni -	added mutex and condition code to synch IT IS WORKING NOW!!! tested down to 256 buffersize using reaper.
30-9-2007: Johnny Petrantoni -	added JackPilot preferences for virtual-ports and auto connect, added some macros for linux compilation, 
								removed prepath from libjack.
2-10-2007: Johnny Petrantoni - added more Linux compatibility code, it should compile now on a linux box (still need edit makefile)

TODO:
1) We should really free(x) memory of asio buffers.
*/

#include "config.h"
#include "port.h"

#include "settings.h"

#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <stdlib.h>

#include <wine/windows/windef.h>
#include <wine/windows/winbase.h>
#include <wine/windows/objbase.h>
#include <wine/windows/mmsystem.h>
#include <wine/windows/psapi.h>

#include <sched.h>
#include <pthread.h>

#include "wine/library.h"
#include "wine/debug.h"

#ifdef __LINUX__
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#else
#include <Jack/jack.h>
#include <Jack/ringbuffer.h>
#endif

#define IEEE754_64FLOAT 1
#include "asio.h"

#ifndef __LINUX__
#include "pThreadUtilities.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(asio);

#ifndef SONAME_LIBJACK
	#ifndef __LINUX__
		#define SONAME_LIBJACK "libjack.dylib"
	#else 
		#define SONAME_LIBJACK "libjack.so"
	#endif
#endif

#define MAKE_FUNCPTR(f) static typeof(f) * fp_##f = NULL;

/* Function pointers for dynamic loading of libjack */
/* these are prefixed with "fp_", ie. "fp_jack_client_new" */
MAKE_FUNCPTR(jack_activate);
MAKE_FUNCPTR(jack_deactivate);
MAKE_FUNCPTR(jack_connect);
MAKE_FUNCPTR(jack_client_open);
MAKE_FUNCPTR(jack_client_close);
MAKE_FUNCPTR(jack_transport_query);
MAKE_FUNCPTR(jack_transport_start);
MAKE_FUNCPTR(jack_set_process_callback);
MAKE_FUNCPTR(jack_get_sample_rate);
MAKE_FUNCPTR(jack_port_register);
MAKE_FUNCPTR(jack_port_get_buffer);
MAKE_FUNCPTR(jack_get_ports);
MAKE_FUNCPTR(jack_port_name);
MAKE_FUNCPTR(jack_get_buffer_size);
MAKE_FUNCPTR(jack_ringbuffer_free);
MAKE_FUNCPTR(jack_ringbuffer_create);
MAKE_FUNCPTR(jack_ringbuffer_write);
MAKE_FUNCPTR(jack_ringbuffer_read);
#undef MAKE_FUNCPTR

void *jackhandle = NULL;

/* JACK callback function */
static int jack_process(jack_nframes_t nframes, void * arg);

/* WIN32 callback function */
static DWORD CALLBACK win32_callback(LPVOID arg);

/* {48D0C522-BFCC-45cc-8B84-17F25F33E6E8} */
static GUID const CLSID_WineASIO = {
0x48d0c522, 0xbfcc, 0x45cc, { 0x8b, 0x84, 0x17, 0xf2, 0x5f, 0x33, 0xe6, 0xe8 } };

#define twoRaisedTo32           4294967296.0
#define twoRaisedTo32Reciprocal	(1.0 / twoRaisedTo32)

unsigned int MAX_INPUTS = 2;
unsigned int MAX_OUTPUTS = 2;
int gAUTO_CONNECT = FALSE;

/* ASIO drivers use the thiscall calling convention which only Microsoft compilers
 * produce.  These macros add an extra layer to fixup the registers properly for
 * this calling convention.
 */

#ifdef __i386__  /* thiscall functions are i386-specific */

#ifdef __GNUC__
/* GCC erroneously warns that the newly wrapped function
 * isn't used, lets help it out of it's thinking
 */
#define SUPPRESS_NOTUSED __attribute__((used))
#else
#define SUPPRESS_NOTUSED
#endif /* __GNUC__ */

#define WRAP_THISCALL(type, func, parm) \
    extern type func parm; \
    __ASM_GLOBAL_FUNC( func, \
                      "popl %eax\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME("__wrapped_" #func) ); \
    SUPPRESS_NOTUSED static type __wrapped_ ## func parm
#else
#define WRAP_THISCALL(functype, function, param) \
    functype function param
#endif

/*****************************************************************************
 * IWineAsio interface
 */
#define INTERFACE IWineASIO
DECLARE_INTERFACE_(IWineASIO,IUnknown) {
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD_(long,init)(THIS_ void *sysHandle) PURE;
    STDMETHOD_(void,getDriverName)(THIS_ char *name) PURE;
    STDMETHOD_(long,getDriverVersion)(THIS) PURE;
    STDMETHOD_(void,getErrorMessage)(THIS_ char *string) PURE;
    STDMETHOD_(ASIOError,start)(THIS) PURE;
    STDMETHOD_(ASIOError,stop)(THIS) PURE;
    STDMETHOD_(ASIOError,getChannels)(THIS_ long *numInputChannels, long *numOutputChannels) PURE;
    STDMETHOD_(ASIOError,getLatencies)(THIS_ long *inputLatency, long *outputLatency) PURE;
    STDMETHOD_(ASIOError,getBufferSize)(THIS_ long *minSize, long *maxSize, long *preferredSize, long *granularity) PURE;
    STDMETHOD_(ASIOError,canSampleRate)(THIS_ ASIOSampleRate sampleRate) PURE;
    STDMETHOD_(ASIOError,getSampleRate)(THIS_ ASIOSampleRate *sampleRate) PURE;
    STDMETHOD_(ASIOError,setSampleRate)(THIS_ ASIOSampleRate sampleRate) PURE;
    STDMETHOD_(ASIOError,getClockSources)(THIS_ ASIOClockSource *clocks, long *numSources) PURE;
    STDMETHOD_(ASIOError,setClockSource)(THIS_ long reference) PURE;
    STDMETHOD_(ASIOError,getSamplePosition)(THIS_ ASIOSamples *sPos, ASIOTimeStamp *tStamp) PURE;
    STDMETHOD_(ASIOError,getChannelInfo)(THIS_ ASIOChannelInfo *info) PURE;
    STDMETHOD_(ASIOError,createBuffers)(THIS_ ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks) PURE;
    STDMETHOD_(ASIOError,disposeBuffers)(THIS) PURE;
    STDMETHOD_(ASIOError,controlPanel)(THIS) PURE;
    STDMETHOD_(ASIOError,future)(THIS_ long selector,void *opt) PURE;
    STDMETHOD_(ASIOError,outputReady)(THIS) PURE;
};
#undef INTERFACE

typedef struct IWineASIO *LPWINEASIO, **LPLPWINEASIO;

enum {
    Init,
    Run,
    Exit
};

typedef struct _Channel {
   ASIOBool active;
   int *buffer;
   jack_ringbuffer_t *ring;
   jack_port_t *port;
} Channel;

struct IWineASIOImpl {
    /* COM stuff */
    const IWineASIOVtbl *lpVtbl;
    LONG                ref;

    /* ASIO stuff */
    HWND                hwnd;
    ASIOSampleRate      sample_rate;
    long                input_latency;
    long                output_latency;
    long                block_frames;
    ASIOTime            asio_time;
    long                miliseconds;
    ASIOTimeStamp       system_time;
    double              sample_position;
    ASIOBufferInfo      *bufferInfos;
    ASIOCallbacks       *callbacks;
    char                error_message[256];
    BOOL                time_info_mode;
    BOOL                tc_read;
    long                state;

    /* JACK stuff */
    jack_client_t       *client;
    long                client_state;
    long                toggle;

    /* callback stuff */
    HANDLE              thread;
    HANDLE              start_event;
    HANDLE              stop_event;
    DWORD               thread_id;
    BOOL                terminate;
    Channel             *input;
    Channel             *output;
	float				*tempbuf;
	pthread_cond_t		cond;
	pthread_mutex_t		mutex;
} This;

typedef struct IWineASIOImpl              IWineASIOImpl;

static void ReadJPPrefs() {
	char *envar;
    char path[256];
	
	envar = getenv("HOME");

	sprintf(path, "%s/Library/Preferences/JAS.jpil", envar);
	FILE *prefFile;
	if ((prefFile = fopen(path, "rt"))) {
		int nullo;
		int input, output, autoconnect, debug, default_input, default_output, default_system;
		fscanf(
				prefFile, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				&input,
				&nullo,
				&output,
				&nullo,
				&autoconnect,
				&nullo,
				&default_input, 
				&nullo,
				&default_output, 
				&nullo,
				&default_system,
				&nullo,
				&debug,
				&nullo,
				&nullo
			);

		fclose(prefFile);
				
		MAX_INPUTS = input;
		MAX_OUTPUTS = output;
		gAUTO_CONNECT = autoconnect;
	}
}

static int GetEXEName(DWORD dwProcessID, char* name) {
    DWORD aProcesses [1024], cbNeeded, cProcesses;
    unsigned int i;
        
    //Enumerate all processes
    if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) return FALSE;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    char szEXEName[MAX_PATH];
    //Loop through all process to find the one that matches
    //the one we are looking for

    for (i = 0; i < cProcesses; i++) {
        if (aProcesses [i] == dwProcessID) {
            // Get a handle to the process
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                              PROCESS_VM_READ, FALSE, dwProcessID);
        
            // Get the process name
            if (NULL != hProcess) {
                HMODULE hMod;
                DWORD cbNeeded;
            
                if(EnumProcessModules(hProcess, &hMod,sizeof(hMod), &cbNeeded)) {
                    //Get the name of the exe file
                    GetModuleBaseNameA(hProcess,hMod,szEXEName,sizeof(szEXEName)/sizeof(char));
					int len = strlen((char*)szEXEName) - 3; // remove ".exe"
					lstrcpynA(name,(char*)szEXEName,len); 
					name[len] = '\0';
					return TRUE;
                 }
            }
        }    
    }

    return FALSE;
}

static ULONG WINAPI IWineASIOImpl_AddRef(LPWINEASIO iface) {
    ULONG ref = InterlockedIncrement(&(This.ref));
    TRACE("(%p) ref was %x\n", &This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineASIOImpl_Release(LPWINEASIO iface) {
    ULONG ref = InterlockedDecrement(&(This.ref));
    TRACE("(%p) ref was %x\n", &This, ref + 1);

    if (!ref) {
		This.terminate = TRUE;
		
		pthread_mutex_lock(&This.mutex);
		pthread_cond_signal(&This.cond);
		pthread_mutex_unlock(&This.mutex);

        WaitForSingleObject(This.stop_event, INFINITE);
	
        fp_jack_client_close(This.client);
        TRACE("JACK client closed\n");
	
        wine_dlclose(jackhandle, NULL, 0);
        jackhandle = NULL;
		
		int i;
		
		for (i=0; i<MAX_INPUTS; i++) {
			fp_jack_ringbuffer_free(This.input[i].ring);
		}
		
		for (i=0; i<MAX_OUTPUTS; i++) {
			fp_jack_ringbuffer_free(This.output[i].ring);
		}
		
		pthread_mutex_destroy(&This.mutex);
		pthread_cond_destroy(&This.cond);
    }

    return ref;
}

static HRESULT WINAPI IWineASIOImpl_QueryInterface(LPWINEASIO iface, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL)
        return E_INVALIDARG;

    if (IsEqualIID(&CLSID_WineASIO, riid)) {
        This.ref++;
        *ppvObject = &This;

        return S_OK;
    }

    return E_NOINTERFACE;
}

WRAP_THISCALL( ASIOBool __stdcall, IWineASIOImpl_init, (LPWINEASIO iface, void *sysHandle)) {
    jack_status_t status;
    int i,j;
    char *envi;
    char name[32];

    This.sample_rate = 48000.0;
    This.block_frames = 1024;
    This.input_latency = This.block_frames;
    This.output_latency = This.block_frames * 2;
    This.miliseconds = (long)((double)(This.block_frames * 1000) / This.sample_rate);
    This.callbacks = NULL;
    This.sample_position = 0;
    strcpy(This.error_message, "No Error");
    This.toggle = 1;
    This.client_state = Init;
    This.time_info_mode = FALSE;
    This.tc_read = FALSE;
    This.terminate = FALSE;
    This.state = Init;
	
	char proc_name[256];
	memset(&proc_name[0],0x0,sizeof(char)*256); //size of char is redundant.. but dunno i like to write it:)
	
	if(!GetEXEName(GetCurrentProcessId(),&proc_name[0])) {
		strcpy(&proc_name[0],"Wine_ASIO");
	}
	
    This.client = fp_jack_client_open(&proc_name[0],JackNullOption, &status, NULL);
    if (This.client == NULL) {
        WARN("failed to open jack server\n");
        return ASIOFalse;
    }

    TRACE("JACK client opened\n");
	
	pthread_mutex_init(&This.mutex,NULL);
	pthread_cond_init(&This.cond,NULL);

    This.sample_rate = fp_jack_get_sample_rate(This.client);
    This.block_frames = fp_jack_get_buffer_size(This.client);
    This.input_latency = This.block_frames;
    This.output_latency = This.block_frames * 2;
    This.miliseconds = (long)((double)(This.block_frames * 1000) / This.sample_rate);

    TRACE("sample rate: %f\n", This.sample_rate);
	
#ifndef __LINUX__
	ReadJPPrefs();
#else
    envi = getenv(ENV_INPUTS);
	if (envi != NULL) MAX_INPUTS = atoi(envi);
    envi = getenv(ENV_OUTPUTS);
	if (envi != NULL) MAX_OUTPUTS = atoi(envi);
#endif

    // initialize input buffers

    This.input = HeapAlloc(GetProcessHeap(), 0, sizeof(Channel) * MAX_INPUTS);
    for (i=0; i<MAX_INPUTS; i++) {
        
        This.input[i].active = ASIOFalse;
        This.input[i].buffer = HeapAlloc(GetProcessHeap(), 0, 2 * This.block_frames * sizeof(int));
        for (j=0; j<This.block_frames * 2; j++)
            This.input[i].buffer[j] = 0;
        This.input[i].port = 0;
		
		This.input[i].ring = fp_jack_ringbuffer_create(4 * This.block_frames * sizeof(float));
    }

    // initialize output buffers

    This.output =  HeapAlloc(GetProcessHeap(), 0, sizeof(Channel) * MAX_OUTPUTS);
    for (i=0; i<MAX_OUTPUTS; i++) {
        This.output[i].active = ASIOFalse;
        This.output[i].buffer = HeapAlloc(GetProcessHeap(), 0, 2 * This.block_frames * sizeof(int));
        for (j=0; j<2*This.block_frames; j++)
            This.output[i].buffer[j] = 0;
        This.output[i].port = 0;
		
		This.output[i].ring = fp_jack_ringbuffer_create(4 * This.block_frames * sizeof(float));
    }
	
	This.tempbuf = HeapAlloc(GetProcessHeap(),0,This.block_frames * sizeof(float));

    This.start_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    This.stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    This.thread = CreateThread(NULL, 0, win32_callback, &This, 0, &This.thread_id);
    if (This.thread) {
        WaitForSingleObject(This.start_event, INFINITE);
        CloseHandle(This.start_event);
        This.start_event = INVALID_HANDLE_VALUE;
    }
    else {
        WARN("Couldn't create thread\n");
        return ASIOFalse;
    }

    if (status & JackServerStarted)
        TRACE("JACK server started\n");

    fp_jack_set_process_callback(This.client, jack_process, &This);

    for (i = 0; i < MAX_INPUTS; i++) {
        sprintf(name, "Input%d", i);
        This.input[i].port = fp_jack_port_register(This.client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,0);
    }

    for (i = 0; i < MAX_OUTPUTS; i++) {
        sprintf(name, "Output%d", i);
        This.output[i].port = fp_jack_port_register(This.client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,0);
    }

    return ASIOTrue;
}

WRAP_THISCALL( void __stdcall, IWineASIOImpl_getDriverName, (LPWINEASIO iface, char *name)) {
    strcpy(name, "Wine ASIO");
}

WRAP_THISCALL( long __stdcall, IWineASIOImpl_getDriverVersion, (LPWINEASIO iface)) {
    return 2;
}

WRAP_THISCALL( void __stdcall, IWineASIOImpl_getErrorMessage, (LPWINEASIO iface, char *string)) {
    strcpy(string, This.error_message);
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_start, (LPWINEASIO iface)) {
    char *var_port = NULL;
    char *val_port = NULL;
    const char ** ports;
    int numports;
    int i;

    if (This.callbacks) {
        This.sample_position = 0;
        This.system_time.lo = 0;
        This.system_time.hi = 0;

        if (fp_jack_activate(This.client)) {
            WARN("couldn't activate client\n");
            return ASE_NotPresent;
        }
		
		if(gAUTO_CONNECT) {
			ports = fp_jack_get_ports(This.client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
			for(numports = 0; ports && ports[numports]; numports++);
			
			for (i = 0; i < MAX_INPUTS; i++) {
				asprintf(&var_port, "%s%d", MAP_INPORT, i);
				val_port = getenv(var_port);
				TRACE("%d: %s %s\n", i, var_port, val_port);
				free(var_port);
				var_port = NULL;

				if (This.input[i].active == ASIOTrue && (val_port || i < numports)) {
					if (fp_jack_connect(This.client,val_port ? val_port : ports[i],fp_jack_port_name(This.input[i].port))) {
						WARN("input %d connect failed\n", i);
					}
				}
			}
			if (ports) free(ports);

			ports = fp_jack_get_ports(This.client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
			for(numports = 0; ports && ports[numports]; numports++);

			for (i = 0; (i < MAX_OUTPUTS); i++) {
				asprintf(&var_port, "%s%d", MAP_OUTPORT, i);
				val_port = getenv(var_port);
				TRACE("%d: %s %s\n", i, var_port, val_port);
				free(var_port);
				var_port = NULL;
               
				if (This.output[i].active == ASIOTrue && (val_port || i < numports)) {
					if (fp_jack_connect(This.client,fp_jack_port_name(This.output[i].port),val_port ? val_port : ports[i])) {
						WARN("output %d connect failed\n", i);
					}
				}
			}
        
			if (ports) free(ports);
		}

        This.state = Run;
        TRACE("started\n");

        return ASE_OK;
    }

    return ASE_NotPresent;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_stop, (LPWINEASIO iface)) {
    This.state = Exit;

    if (fp_jack_deactivate(This.client)) {
        WARN("couldn't deactivate client\n");
        return ASE_NotPresent;
    }

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getChannels, (LPWINEASIO iface, long *numInputChannels, long *numOutputChannels)) {
    if (numInputChannels)
        *numInputChannels = MAX_INPUTS;

    if (numOutputChannels)
        *numOutputChannels = MAX_OUTPUTS;

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getLatencies, (LPWINEASIO iface, long *inputLatency, long *outputLatency)) {
    if (inputLatency)
        *inputLatency = This.input_latency;

    if (outputLatency)
        *outputLatency = This.output_latency;

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getBufferSize, (LPWINEASIO iface, long *minSize, long *maxSize, long *preferredSize, long *granularity)) {
    if (minSize)
        *minSize = This.block_frames;

    if (maxSize)
        *maxSize = This.block_frames;

    if (preferredSize)
        *preferredSize = This.block_frames;

    if (granularity)
        *granularity = 0;

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_canSampleRate, (LPWINEASIO iface, ASIOSampleRate sampleRate)) {
    if (sampleRate == This.sample_rate)
        return ASE_OK;

    return ASE_NoClock;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getSampleRate, (LPWINEASIO iface, ASIOSampleRate *sampleRate)) {
    if (sampleRate)
        *sampleRate = This.sample_rate;

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_setSampleRate, (LPWINEASIO iface, ASIOSampleRate sampleRate)) {
    if (sampleRate != This.sample_rate)
        return ASE_NoClock;

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getClockSources, (LPWINEASIO iface, ASIOClockSource *clocks, long *numSources)) {
    if (clocks && numSources) {
        clocks->index = 0;
        clocks->associatedChannel = -1;
        clocks->associatedGroup = -1;
        clocks->isCurrentSource = ASIOTrue;
        strcpy(clocks->name, "Internal");

        *numSources = 1;
        return ASE_OK;
    }

    return ASE_InvalidParameter;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_setClockSource, (LPWINEASIO iface, long reference)) {
    if (reference == 0) {
        This.asio_time.timeInfo.flags |= kClockSourceChanged;

        return ASE_OK;
    }

    return ASE_NotPresent;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getSamplePosition, (LPWINEASIO iface, ASIOSamples *sPos, ASIOTimeStamp *tStamp)) {
    tStamp->lo = This.system_time.lo;
    tStamp->hi = This.system_time.hi;

    if (This.sample_position >= twoRaisedTo32) {
        sPos->hi = (unsigned long)(This.sample_position * twoRaisedTo32Reciprocal);
        sPos->lo = (unsigned long)(This.sample_position - (sPos->hi * twoRaisedTo32));
    }
    else {
        sPos->hi = 0;
        sPos->lo = (unsigned long)This.sample_position;
    }

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_getChannelInfo, (LPWINEASIO iface, ASIOChannelInfo *info)) {
    char name[32];

    if (info->channel < 0 || (info->isInput ? info->channel >= MAX_INPUTS : info->channel >= MAX_OUTPUTS))
        return ASE_InvalidParameter;

//    info->type = ASIOSTFloat32LSB;
    info->type = ASIOSTInt32LSB;
    info->channelGroup = 0;

    if (info->isInput) {
        info->isActive = This.input[info->channel].active;
        sprintf(name, "Input %ld", info->channel);
    }
    else {
        info->isActive = This.output[info->channel].active;
        sprintf(name, "Output %ld", info->channel);
    }

    strcpy(info->name, name);

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_disposeBuffers, (LPWINEASIO iface)) {
    int i;

    This.callbacks = NULL;
    __wrapped_IWineASIOImpl_stop(iface);

    for (i = 0; i < MAX_INPUTS; i++) {
        This.input[i].active = ASIOFalse;
    }

    for (i = 0; i < MAX_OUTPUTS; i++) {
        This.output[i].active = ASIOFalse;
    }

    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_createBuffers, (LPWINEASIO iface, ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks)) {
    ASIOBufferInfo * info = bufferInfos;
    int i;

    This.block_frames = bufferSize;
    This.miliseconds = (long)((double)(This.block_frames * 1000) / This.sample_rate);

    for (i = 0; i < numChannels; i++, info++) {
        if (info->isInput) {
            if (info->channelNum < 0 || info->channelNum >= MAX_INPUTS) {
                WARN("invalid input channel: %ld\n", info->channelNum);
                goto ERROR_PARAM;
            }

            This.input[info->channelNum].active = ASIOTrue;
            info->buffers[0] = &This.input[info->channelNum].buffer[0];
            info->buffers[1] = &This.input[info->channelNum].buffer[This.block_frames];
        }
        else {
            if (info->channelNum < 0 || info->channelNum >= MAX_OUTPUTS) {
                WARN("invalid output channel: %ld\n", info->channelNum);
                goto ERROR_PARAM;
            }

            This.output[info->channelNum].active = ASIOTrue;
            info->buffers[0] = &This.output[info->channelNum].buffer[0];
            info->buffers[1] = &This.output[info->channelNum].buffer[This.block_frames];
        }
    }

    This.callbacks = callbacks;

    if (This.callbacks->asioMessage) {
        if (This.callbacks->asioMessage(kAsioSupportsTimeInfo, 0, 0, 0)) {
            This.time_info_mode = TRUE;
            This.asio_time.timeInfo.speed = 1;
            This.asio_time.timeInfo.systemTime.hi = 0;
            This.asio_time.timeInfo.systemTime.lo = 0;
            This.asio_time.timeInfo.samplePosition.hi = 0;
            This.asio_time.timeInfo.samplePosition.lo = 0;
            This.asio_time.timeInfo.sampleRate = This.sample_rate;
            This.asio_time.timeInfo. flags = kSystemTimeValid | kSamplePositionValid | kSampleRateValid;

            This.asio_time.timeCode.speed = 1;
            This.asio_time.timeCode.timeCodeSamples.hi = 0;
            This.asio_time.timeCode.timeCodeSamples.lo = 0;
            This.asio_time.timeCode.flags = kTcValid | kTcRunning;
        }
        else
            This.time_info_mode = FALSE;
    }
    else {
        This.time_info_mode = FALSE;
        WARN("asioMessage callback not supplied\n");
        goto ERROR_PARAM;
    }

    return ASE_OK;

ERROR_PARAM:
    __wrapped_IWineASIOImpl_disposeBuffers(iface);
    WARN("invalid parameter\n");
    return ASE_InvalidParameter;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_controlPanel, (LPWINEASIO iface)) {
    return ASE_OK;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_future, (LPWINEASIO iface, long selector, void *opt)) {

    switch (selector)
    {
    case kAsioEnableTimeCodeRead:
        This.tc_read = TRUE;
        return ASE_SUCCESS;
    case kAsioDisableTimeCodeRead:
        This.tc_read = FALSE;
        return ASE_SUCCESS;
    case kAsioSetInputMonitor:
        return ASE_SUCCESS;
    case kAsioCanInputMonitor:
        return ASE_SUCCESS;
    case kAsioCanTimeInfo:
        return ASE_SUCCESS;
    case kAsioCanTimeCode:
        return ASE_SUCCESS;
    }

    return ASE_NotPresent;
}

WRAP_THISCALL( ASIOError __stdcall, IWineASIOImpl_outputReady, (LPWINEASIO iface)) {
    return ASE_NotPresent;
}

static const IWineASIOVtbl WineASIO_Vtbl =
{
    IWineASIOImpl_QueryInterface,
    IWineASIOImpl_AddRef,
    IWineASIOImpl_Release,
    IWineASIOImpl_init,
    IWineASIOImpl_getDriverName,
    IWineASIOImpl_getDriverVersion,
    IWineASIOImpl_getErrorMessage,
    IWineASIOImpl_start,
    IWineASIOImpl_stop,
    IWineASIOImpl_getChannels,
    IWineASIOImpl_getLatencies,
    IWineASIOImpl_getBufferSize,
    IWineASIOImpl_canSampleRate,
    IWineASIOImpl_getSampleRate,
    IWineASIOImpl_setSampleRate,
    IWineASIOImpl_getClockSources,
    IWineASIOImpl_setClockSource,
    IWineASIOImpl_getSamplePosition,
    IWineASIOImpl_getChannelInfo,
    IWineASIOImpl_createBuffers,
    IWineASIOImpl_disposeBuffers,
    IWineASIOImpl_controlPanel,
    IWineASIOImpl_future,
    IWineASIOImpl_outputReady,
};

BOOL init_jack() {
    jackhandle = wine_dlopen(SONAME_LIBJACK, RTLD_NOW, NULL, 0);
    TRACE("SONAME_LIBJACK == %s\n", SONAME_LIBJACK);
    TRACE("jackhandle == %p\n", jackhandle);
    if (!jackhandle)
    {
        FIXME("error loading the jack library %s, please install this library to use jack\n", SONAME_LIBJACK);
        jackhandle = (void*)-1;
        return FALSE;
    }

    /* setup function pointers */
#define LOAD_FUNCPTR(f) if((fp_##f = wine_dlsym(jackhandle, #f, NULL, 0)) == NULL) goto sym_not_found;
    LOAD_FUNCPTR(jack_activate);
    LOAD_FUNCPTR(jack_deactivate);
    LOAD_FUNCPTR(jack_connect);
    LOAD_FUNCPTR(jack_client_open);
    LOAD_FUNCPTR(jack_client_close);
    LOAD_FUNCPTR(jack_transport_query);
    LOAD_FUNCPTR(jack_transport_start);
    LOAD_FUNCPTR(jack_set_process_callback);
    LOAD_FUNCPTR(jack_get_sample_rate);
    LOAD_FUNCPTR(jack_port_register);
    LOAD_FUNCPTR(jack_port_get_buffer);
    LOAD_FUNCPTR(jack_get_ports);
    LOAD_FUNCPTR(jack_port_name);
    LOAD_FUNCPTR(jack_get_buffer_size);
	LOAD_FUNCPTR(jack_ringbuffer_free);
	LOAD_FUNCPTR(jack_ringbuffer_create);
	LOAD_FUNCPTR(jack_ringbuffer_write);
	LOAD_FUNCPTR(jack_ringbuffer_read);
#undef LOAD_FUNCPTR

    return TRUE;

    /* error path for function pointer loading errors */
sym_not_found:
    WINE_MESSAGE("Wine cannot find certain functions that it needs inside the jack"
                 "library.  To enable Wine to use the jack audio server please "
                 "install libjack\n");
    wine_dlclose(jackhandle, NULL, 0);
    jackhandle = NULL;
    return FALSE;
}

HRESULT asioCreateInstance(REFIID riid, LPVOID *ppobj) {
    if (!init_jack()) {
        return ERROR_NOT_SUPPORTED;
    }
 
    This.lpVtbl = &WineASIO_Vtbl;
    This.ref = 1;
    *ppobj = &This;
    return S_OK;
}

static void getNanoSeconds(ASIOTimeStamp* ts) {
    double nanoSeconds = (double)((unsigned long)timeGetTime ()) * 1000000.;
    ts->hi = (unsigned long)(nanoSeconds / twoRaisedTo32);
    ts->lo = (unsigned long)(nanoSeconds - (ts->hi * twoRaisedTo32));
}

static int jack_process(jack_nframes_t nframes, void * arg) {
    int i;
    jack_default_audio_sample_t *in, *out;

	if (This.client_state == Init) This.client_state = Run;

	/* get the input data from JACK and copy it to the ASIO buffers */
	for (i = 0; i < MAX_INPUTS; i++) {
		if (This.input[i].active == ASIOTrue) {
			in = fp_jack_port_get_buffer(This.input[i].port, nframes);
			
			fp_jack_ringbuffer_write(This.input[i].ring,in,nframes * sizeof(float));
		}
	}
	
	pthread_mutex_lock(&This.mutex);
	pthread_cond_signal(&This.cond);
	pthread_mutex_unlock(&This.mutex);

	// copy the ASIO data to JACK
	for (i = 0; i < MAX_OUTPUTS; i++) {
		if (This.output[i].active == ASIOTrue) {
			out = fp_jack_port_get_buffer(This.output[i].port, nframes);
			
			fp_jack_ringbuffer_read(This.output[i].ring,out,nframes * sizeof(float));
		}
	}

    return 0;
}

/*
 * The ASIO callback can make WIN32 calls which require a WIN32 thread.
 * Do the callback in this thread and then switch back to the Jack callback thread.
 */
static DWORD CALLBACK win32_callback(LPVOID arg) {
#ifndef __LINUX__
	setThreadToPriority(pthread_self(),96,TRUE,10000000);
#else 
	struct sched_param rtparam;
	
	memset (&rtparam, 0, sizeof (rtparam));
	
	rtparam.sched_priority = 98; // I'm not 100% sure on this...
	
	pthread_setschedparam(pthread_self(),SCHED_FIFO,&rtparam);
#endif

    /* let IWineASIO_Init know we are alive */
    SetEvent(This.start_event);
	
	int *buffer;
	int i,j;
	
	pthread_mutex_lock(&This.mutex);

    while (1) {
		pthread_cond_wait(&This.cond,&This.mutex); //this should (mutex) unlock while waiting and lock automatically when signaled...
	
        /* check for termination */
        if (This.terminate) {
            SetEvent(This.stop_event);
            TRACE("terminated\n");
            return 0;
        }
		
        getNanoSeconds(&This.system_time);

        /* make sure we are in the run state */

        if (This.state == Run) {
			This.sample_position += This.block_frames;
		
			for (i = 0; i < MAX_INPUTS; i++) {
				if (This.input[i].active == ASIOTrue) {
					buffer = &This.input[i].buffer[This.block_frames * This.toggle];
					
					fp_jack_ringbuffer_read(This.input[i].ring,This.tempbuf,This.block_frames * sizeof(float));

					for (j = 0; j < This.block_frames; j++) buffer[j] = (int)(This.tempbuf[j] * (float)(0x7fffffff));
				}
			}
		
            if (This.time_info_mode) {
				__wrapped_IWineASIOImpl_getSamplePosition((LPWINEASIO)&This,&This.asio_time.timeInfo.samplePosition,&This.asio_time.timeInfo.systemTime);
                if (This.tc_read) {
                    This.asio_time.timeCode.timeCodeSamples.lo = This.asio_time.timeInfo.samplePosition.lo;
                    This.asio_time.timeCode.timeCodeSamples.hi = 0;
                }
                This.callbacks->bufferSwitchTimeInfo(&This.asio_time, This.toggle, ASIOTrue);
                This.asio_time.timeInfo.flags &= ~(kSampleRateChanged | kClockSourceChanged);
            }
            else {
                This.callbacks->bufferSwitch(This.toggle,ASIOTrue);
            }
			
			for (i = 0; i < MAX_OUTPUTS; i++) {
				if (This.output[i].active == ASIOTrue) {
					buffer = &This.output[i].buffer[This.block_frames * This.toggle];
					
					for (j = 0; j < This.block_frames; j++) This.tempbuf[j] = ((float)(buffer[j]) / (float)(0x7fffffff));
					
					fp_jack_ringbuffer_write(This.output[i].ring,This.tempbuf,This.block_frames * sizeof(float));
				}
			}
			
			This.toggle = This.toggle ? 0 : 1;
        }
    }
	
	pthread_mutex_unlock(&This.mutex);
    
    return 0;
}
