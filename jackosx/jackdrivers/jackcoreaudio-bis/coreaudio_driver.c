/*
    Copyright © Grame, 2003.
	Copyright © Johnny Petrantoni, 2003.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
    grame@rd.grame.fr
	
	Johnny Petrantoni, johnny@lato-b.com - Italy, Rome.
    
	Jan 30, 2004: Johnny Petrantoni: first code of the coreaudio driver, based on portaudio driver by Stephane Letz.
	Feb 02, 2004: Johnny Petrantoni: fixed null cycle, removed double copy of buffers in AudioRender, the driver works fine (tested with Built-in Audio and Hammerfall RME), 
									 but no cpu load is displayed.
	Feb 03, 2004: Johnny Petrantoni: some little fix.
	Feb 03, 2004: Stephane Letz: some fix in AudioRender.cpp code.
	Feb 03, 2004: Johnny Petrantoni: removed the default device stuff (useless, in jackosx, because JackPilot manages this behavior), 
									 the device must be specified. and all parameter must be correct.
	Feb 04, 2004: Johnny Petrantoni: now the driver supports interfaces with multiple interleaved streams (such as the MOTU 828).
	Feb 13, 2004: Johnny Petrantoni: new driver design based on AUHAL.
	April 7, 2004: Johnny Petrantoni: option -I in order to use AudioDeviceID.
	April 8,2008: S.Letz: fix a bug in GetDeviceNameFromID and a problem with "dirty buffers" when no ouput connections are made.
	May 7, 2004: Johnny Petrantoni: Improved c++ code,cpu load counter fixed(jack_init_time() now links) and fixed GetDeviceNameFromID.
	May 10, 2004: Johnny Petrantoni: Improved "change buffer size" behavior , TO BE TESTED!!!...
										 
	TODO:
	- multiple-device processing.
	
   
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <jack/engine.h>
#include <assert.h>
#include "coreaudio_driver.h"

const int CAVersion = 1;

void JCALog(char *fmt,...);
int coreaudio_runCycle(void *driver,long bufferSize,float **inBuffers,float **outBuffers);

OSStatus GetDeviceNameFromID(AudioDeviceID id, char name[60]) 
{
	UInt32 size = sizeof(char)*60;
	return AudioDeviceGetProperty(id,0,false,kAudioDevicePropertyDeviceName,&size,&name[0]);
}

static int coreaudio_driver_changeBufferSize(coreaudio_driver_t* driver)
{	 
	// First we check if it is possible, and then we close the old audio instance and switch to the new stream instance.
	void *newStream = openAudioInstance((float)driver->device_frame_rate,driver->new_bsize,driver->capturing,driver->playing,&driver->driver_name[0]);
	 
	if(!newStream) return FALSE;
	
	closeAudioInstance(driver->stream);
	
	driver->stream = newStream;
	driver->frames_per_cycle = driver->new_bsize;
	setHostData(driver->stream,driver);
	setCycleFun(driver->stream,coreaudio_runCycle);

	return startAudioProcess(driver->stream);
}

int coreaudio_runCycle(void *driver,long bufferSize,float **inBuffers,float **outBuffers) {
	coreaudio_driver_t * ca_driver = (coreaudio_driver_t*)driver;
	ca_driver->incoreaudio = inBuffers;
	ca_driver->outcoreaudio = outBuffers;
	ca_driver->last_wait_ust = jack_get_microseconds();
	int res = ca_driver->engine->run_cycle(ca_driver->engine, bufferSize, 0);
	
	if(ca_driver->needsChangeBufferSize) {
		coreaudio_driver_changeBufferSize(ca_driver);
		ca_driver->needsChangeBufferSize = FALSE;
	}
	
	return res;
}

static int
coreaudio_driver_attach (coreaudio_driver_t *driver, jack_engine_t *engine)
{
        jack_port_t *port;
        int port_flags;
        channel_t chn;
        char buf[JACK_PORT_NAME_SIZE];
            
        driver->engine = engine;
        
        driver->engine->set_buffer_size (engine, driver->frames_per_cycle);
        driver->engine->set_sample_rate (engine, driver->frame_rate);
        
        port_flags = JackPortIsOutput|JackPortIsPhysical|JackPortIsTerminal;
    
        /*
        if (driver->has_hw_monitoring) {
                port_flags |= JackPortCanMonitor;
        }
        */
    
        for (chn = 0; chn < driver->capture_nchannels; chn++) {
    
                //snprintf (buf, sizeof(buf) - 1, "capture_%lu", chn+1);
                snprintf (buf, sizeof(buf) - 1, "%s:out%lu", driver->driver_name, chn+1);
    
                if ((port = jack_port_register (driver->client, buf, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0)) == NULL) {
                        jack_error ("coreaudio: cannot register port for %s", buf);
                        break;
                }
    
                /* XXX fix this so that it can handle: systemic (external) latency
                */
    
                jack_port_set_latency (port, driver->frames_per_cycle);
    
                driver->capture_ports = jack_slist_append (driver->capture_ports, port);
        }
        
        port_flags = JackPortIsInput|JackPortIsPhysical|JackPortIsTerminal;
    
        for (chn = 0; chn < driver->playback_nchannels; chn++) {
                //snprintf (buf, sizeof(buf) - 1, "playback_%lu", chn+1);
                snprintf (buf, sizeof(buf) - 1, "%s:in%lu", driver->driver_name, chn+1);
    
                if ((port = jack_port_register (driver->client, buf, JACK_DEFAULT_AUDIO_TYPE, port_flags, 0)) == NULL) {
                        jack_error ("coreaudio: cannot register port for %s", buf);
                        break;
                }
                
                /* XXX fix this so that it can handle: systemic (external) latency
                */
        
                jack_port_set_latency (port, driver->frames_per_cycle);
                driver->playback_ports = jack_slist_append (driver->playback_ports, port);
        }
    
        jack_activate (driver->client);
        
        return 0; 
}

static int
coreaudio_driver_detach (coreaudio_driver_t *driver, jack_engine_t *engine)
{
        JSList *node;
    
        if (driver->engine == 0) {
                return -1;
        }
    
        for (node = driver->capture_ports; node; node = jack_slist_next (node)) {
                jack_port_unregister (driver->client, ((jack_port_t *) node->data));
        }
    
        jack_slist_free (driver->capture_ports);
        driver->capture_ports = 0;
                
        for (node = driver->playback_ports; node; node = jack_slist_next (node)) {
                jack_port_unregister (driver->client, ((jack_port_t *) node->data));
        }
    
        jack_slist_free (driver->playback_ports);
        driver->playback_ports = 0;
        
        driver->engine = 0;
        
        return 0; 
}

static int
coreaudio_driver_null_cycle (coreaudio_driver_t* driver, jack_nframes_t nframes)
{
	int i;
	
	for(i=0;i<driver->playback_nchannels;i++) {
		memset(driver->outcoreaudio[i], 0x0,nframes * sizeof(float));
	}
	
	return 0;
}

static int
coreaudio_driver_read (coreaudio_driver_t *driver, jack_nframes_t nframes)
{
        jack_default_audio_sample_t *buf;
        channel_t chn;
        jack_port_t *port;
        JSList *node;
				
        for (chn = 0, node = driver->capture_ports; node; node = jack_slist_next (node), chn++) {
                
                port = (jack_port_t *)node->data;
				
				if (jack_port_connected (port) && (driver->incoreaudio[chn] != NULL)) {
					float *in = driver->incoreaudio[chn];
					buf = jack_port_get_buffer (port, nframes); 
					memcpy(buf,in,sizeof(float)*nframes);
				}
       }
       
        driver->engine->transport_cycle_start (driver->engine,jack_get_microseconds ());
        return 0;
}          

static int
coreaudio_driver_write (coreaudio_driver_t *driver, jack_nframes_t nframes)
{
        jack_default_audio_sample_t *buf;
        channel_t chn;
        jack_port_t *port;
        JSList *node;
				                
		for (chn = 0, node = driver->playback_ports; node; node = jack_slist_next (node), chn++) {
                
                port = (jack_port_t *)node->data;
				
				if (driver->outcoreaudio[chn] != NULL) {
					float *out = driver->outcoreaudio[chn];
					if (jack_port_connected (port)) {
						buf = jack_port_get_buffer (port, nframes);
						memcpy(out,buf,sizeof(float)*nframes);
					}else{
						memset(out,0,sizeof(float)*nframes);
					}
				}
		}
        
        return 0;
}

static int
coreaudio_driver_audio_start (coreaudio_driver_t *driver)
{        
	return (!startAudioProcess(driver->stream)) ? -1 : 0;
}

static int
coreaudio_driver_audio_stop (coreaudio_driver_t *driver)
{
	return (!stopAudioProcess(driver->stream)) ? -1 : 0;
}

static int
coreaudio_driver_bufsize (coreaudio_driver_t* driver, jack_nframes_t nframes)
{

	/* This gets called from the engine server thread, so it must
	 * be serialized with the driver thread.  Stopping the audio
	 * also stops that thread. */
	driver->needsChangeBufferSize = TRUE;
	driver->new_bsize = nframes;
	
	return 0; // is that correct???, if think not...
}


/** create a new driver instance
 */
static jack_driver_t *
coreaudio_driver_new (char *name, 
				jack_client_t* client,
				jack_nframes_t frames_per_cycle,
				jack_nframes_t rate,
				int capturing,
				int playing,
				int chan_in, 
				int chan_out,
				DitherAlgorithm dither,
				char* driver_name,AudioDeviceID deviceID)
{
	coreaudio_driver_t *driver;
	
	JCALog ("coreaudio beta %d driver\n",CAVersion);

	driver = (coreaudio_driver_t *) calloc (1, sizeof (coreaudio_driver_t));

	jack_driver_init ((jack_driver_t *) driver);

	if (!jack_power_of_two(frames_per_cycle)) {
		fprintf (stderr, "PA: -p must be a power of two.\n");
		goto error;
	}

	driver->frames_per_cycle = frames_per_cycle;
	driver->device_frame_rate = rate;
	driver->capturing = capturing;
	driver->playing = playing;
	driver->needsChangeBufferSize = FALSE;
	driver->new_bsize = frames_per_cycle;

	driver->attach = (JackDriverAttachFunction) coreaudio_driver_attach;
	driver->detach = (JackDriverDetachFunction) coreaudio_driver_detach;
	driver->read = (JackDriverReadFunction) coreaudio_driver_read;
	driver->write = (JackDriverReadFunction) coreaudio_driver_write;
	driver->null_cycle = (JackDriverNullCycleFunction) coreaudio_driver_null_cycle;
	driver->bufsize = (JackDriverBufSizeFunction) coreaudio_driver_bufsize;
	driver->start = (JackDriverStartFunction) coreaudio_driver_audio_start;
	driver->stop = (JackDriverStopFunction) coreaudio_driver_audio_stop;
	driver->stream = NULL;
	
	char deviceName[60];
	bzero(&deviceName[0],sizeof(char)*60);
	
	if(!driver_name) {
		if (GetDeviceNameFromID(deviceID,deviceName) != noErr) goto error; 
	} else {
		strcpy(&deviceName[0],driver_name);
	}
		
	driver->stream = openAudioInstance((float)rate,frames_per_cycle,chan_in,chan_out,&deviceName[0]);

	if(!driver->stream) goto error;
		
	driver->client = client; 
	driver->period_usecs = (((float)driver->frames_per_cycle) / driver->device_frame_rate) * 1000000.0f;
	
	setHostData(driver->stream,driver);
	setCycleFun(driver->stream,coreaudio_runCycle);
	
	driver->incoreaudio = NULL;
	driver->outcoreaudio = NULL;
	
	driver->playback_nchannels = chan_out;
	driver->capture_nchannels = chan_in;
	
	strcpy(&driver->driver_name[0],&deviceName[0]);
	
	jack_init_time();
	 
	return((jack_driver_t *) driver);

error:

	JCALog("Cannot open the coreaudio stream\n"); 
	free(driver);
	return NULL;
}

/** free all memory allocated by a driver instance
 */
static void
coreaudio_driver_delete (coreaudio_driver_t *driver)
{
	/* Close coreaudio stream and terminate */
	closeAudioInstance(driver->stream);
	free(driver);
}

//== driver "plugin" interface =================================================

/* DRIVER "PLUGIN" INTERFACE */

jack_driver_desc_t *
driver_get_descriptor ()
{
	jack_driver_desc_t * desc;
	unsigned int i;
	desc = calloc (1, sizeof (jack_driver_desc_t));

	strcpy (desc->name, "coreaudio");
	desc->nparams = 11;
	desc->params = calloc (desc->nparams,
			       sizeof (jack_driver_param_desc_t));

	i = 0;
	strcpy (desc->params[i].name, "channel");
	desc->params[i].character  = 'c';
	desc->params[i].type       = JackDriverParamInt;
	desc->params[i].value.ui   = 0;
	strcpy (desc->params[i].short_desc, "Maximium number of channels");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);
	
	i++;
	strcpy (desc->params[i].name, "channelin");
	desc->params[i].character  = 'i';
	desc->params[i].type       = JackDriverParamInt;
	desc->params[i].value.ui   = 0;
	strcpy (desc->params[i].short_desc, "Maximium number of input channels");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "channelout");
	desc->params[i].character  = 'o';
	desc->params[i].type       = JackDriverParamInt;
	desc->params[i].value.ui   = 0;
	strcpy (desc->params[i].short_desc, "Maximium number of ouput channels");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "capture");
	desc->params[i].character  = 'C';
	desc->params[i].type       = JackDriverParamBool;
	desc->params[i].value.i    = TRUE;
	strcpy (desc->params[i].short_desc, "Whether or not to capture");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "playback");
	desc->params[i].character  = 'P';
	desc->params[i].type       = JackDriverParamBool;
	desc->params[i].value.i    = TRUE;
	strcpy (desc->params[i].short_desc, "Whether or not to playback");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "duplex");
	desc->params[i].character  = 'D';
	desc->params[i].type       = JackDriverParamBool;
	desc->params[i].value.i    = TRUE;
	strcpy (desc->params[i].short_desc, "Capture and playback");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "rate");
	desc->params[i].character  = 'r';
	desc->params[i].type       = JackDriverParamUInt;
	desc->params[i].value.ui   = 48000U;
	strcpy (desc->params[i].short_desc, "Sample rate");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "period");
	desc->params[i].character  = 'p';
	desc->params[i].type       = JackDriverParamUInt;
	desc->params[i].value.ui   = 128U;
	strcpy (desc->params[i].short_desc, "Frames per period");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);
	
	i++;
	strcpy (desc->params[i].name, "device ID");
	desc->params[i].character  = 'I';
	desc->params[i].type       = JackDriverParamUInt;
	desc->params[i].value.ui   = 0U;
	strcpy (desc->params[i].short_desc, "AudioDeviceID of device");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);
	
	i++;
	strcpy (desc->params[i].name, "name");
	desc->params[i].character  = 'n';
	desc->params[i].type       = JackDriverParamString;
	desc->params[i].value.ui   = 128U;
	strcpy (desc->params[i].short_desc, "Driver name");
	strcpy (desc->params[i].long_desc, desc->params[i].short_desc);

	i++;
	strcpy (desc->params[i].name, "dither");
	desc->params[i].character  = 'z';
	desc->params[i].type       = JackDriverParamChar;
	desc->params[i].value.c    = '-';
	strcpy (desc->params[i].short_desc, "Dithering mode");
	strcpy (desc->params[i].long_desc,
		"  Dithering Mode:\n"
		"    r : rectangular\n"
		"    t : triangular\n"
		"    s : shaped\n"
		"    - : no dithering");
	return desc;
}

const char driver_client_name[] = "coreaudio";
jack_driver_t *
driver_initialize (jack_client_t *client, const JSList * params)
{
	jack_nframes_t srate = 44100;
	jack_nframes_t frames_per_interrupt = 512;
	
	int capture = FALSE;
	int playback = FALSE;
	int chan_in = -1;
	int chan_out = -1;
	DitherAlgorithm dither = None;
	const JSList * node;
	const jack_driver_param_t * param;
	char* name = NULL;
	AudioDeviceID deviceID = 0;

	for (node = params; node; node = jack_slist_next (node)) {
		param = (const jack_driver_param_t *) node->data;

		switch (param->character) {
                
		case 'n':
			name =  (char*)param->value.str;
			printf("Driver name found %s\n",name);
			break;
                        
		case 'D':
			capture = TRUE;
			playback = TRUE;
			break;
                                    
		case 'c':
			chan_in = (int) param->value.ui;
			chan_out = (int) param->value.ui;
			break;
			
		case 'i':
			chan_in = (int) param->value.ui;
			break;
		
		case 'o':
			chan_out = (int) param->value.ui;
			break;
	 
		case 'C':
			capture = param->value.i;
			break;
    
		case 'P':
			playback = param->value.i;
			break;

		case 'r':
			srate = param->value.ui;
			break;
                                    
		case 'p':
			frames_per_interrupt = (unsigned int) param->value.ui;
			break;
		
		case 'I':
			deviceID = (AudioDeviceID) param->value.ui;
			break;
                                    
		case 'z':
			switch ((int) param->value.c) {
			case '-':
				dither = None;
				break;
    
			case 'r':
				dither = Rectangular;
				break;
    
			case 's':
				dither = Shaped;
				break;
    
			case 't':
			default:
				dither = Triangular;
				break;
			}
			break;
		}
	}

        /* duplex is the default */
	if (!capture && !playback) {
		capture = TRUE;
		playback = TRUE;
	}

	return coreaudio_driver_new ("coreaudio", client, frames_per_interrupt,
				     srate, capture, playback, chan_in, chan_out, dither,name,deviceID);
}


void
driver_finish (jack_driver_t *driver)
{
	coreaudio_driver_delete ((coreaudio_driver_t *) driver);
}


