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
									 
	TODO:
	- fix cpu load behavior.
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

OSStatus GetDeviceNameFromID(AudioDeviceID id, char name[60]) 
{
	UInt32 size = sizeof(char)*60;
	return AudioDeviceGetProperty(id,0,false,kAudioDevicePropertyDeviceName,&size,&name[0]);
}

int coreaudio_runCycle(void *driver,long bufferSize) {
	coreaudio_driver_t * ca_driver = (coreaudio_driver_t*)driver;
	ca_driver->last_wait_ust = jack_get_microseconds();
	return ca_driver->engine->run_cycle(ca_driver->engine, bufferSize, 0);
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
	
	if(!driver->isInterleaved) {
		for(i=0;i<driver->playback_nchannels;i++) {
			memset(driver->outcoreaudio[i], 0x0,nframes * sizeof(float));
		}
	} else {
		memset(driver->outcoreaudio[0], 0x0,nframes * sizeof(float) * driver->playback_nchannels);
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
		int i;
		
		int b = 0;
		
        for (chn = 0, node = driver->capture_ports; node; node = jack_slist_next (node), chn++) {
                
                port = (jack_port_t *)node->data;
				
				if(!driver->isInterleaved) {
					if (jack_port_connected (port) && (driver->incoreaudio[chn] != NULL)) {
						float *in = driver->incoreaudio[chn];
						buf = jack_port_get_buffer (port, nframes); 
						memcpy(buf,in,sizeof(float)*nframes);
					}
				} else {
					if (jack_port_connected (port) && (driver->incoreaudio[b] != NULL)) {
						int channels = driver->channelsPerStream[b];
						if(channels<=chn) {
							b++;
							if(driver->numberOfStreams>1 && b<driver->numberOfStreams) {
								channels = driver->channelsPerStream[b];
								chn = 0;
							} else return 0;
						}
						if(channels>0) {
							float *in = driver->incoreaudio[b];
							buf = jack_port_get_buffer (port, nframes); 
							for (i = 0; i< nframes; i++) buf[i] = in[channels*i+chn];
						}
					}
				}
    
        }
       
        driver->engine->transport_cycle_start (driver->engine,
					       jack_get_microseconds ());
        return 0;
}          

static int
coreaudio_driver_write (coreaudio_driver_t *driver, jack_nframes_t nframes)
{
        jack_default_audio_sample_t *buf;
        channel_t chn;
        jack_port_t *port;
        JSList *node;
		int i;
		
		int b = 0;
		                
        for (chn = 0, node = driver->playback_ports; node; node = jack_slist_next (node), chn++) {
                
                port = (jack_port_t *)node->data;
				
                if(!driver->isInterleaved) {
					if (jack_port_connected (port) && (driver->outcoreaudio[chn] != NULL)) {
						float *out = driver->outcoreaudio[chn];
                        buf = jack_port_get_buffer (port, nframes);
						memcpy(out,buf,sizeof(float)*nframes);
					}
				} else {
					if (jack_port_connected (port) && (driver->outcoreaudio[b] != NULL)) {
						int channels = driver->out_channelsPerStream[b];
						if(channels<=chn) {
							b++;
							if(driver->out_numberOfStreams>1 && b<driver->out_numberOfStreams) {
								channels = driver->out_channelsPerStream[b];
								chn = 0;
							} else return 0;
						}
						if(channels>0) {
							float *out = driver->outcoreaudio[b];
							buf = jack_port_get_buffer (port, nframes);
							for (i = 0; i< nframes; i++) out[channels*i+chn] = buf[i];
						}
					}
				}
        }
        
        return 0;
}

static int
coreaudio_driver_audio_start (coreaudio_driver_t *driver)
{
        
	return (!startPandaAudioProcess(driver->stream)) ? -1 : 0;
}

static int
coreaudio_driver_audio_stop (coreaudio_driver_t *driver)
{
        return (!stopPandaAudioProcess(driver->stream)) ? -1 : 0;
}

#if 0
static int
coreaudio_driver_bufsize (coreaudio_driver_t* driver, jack_nframes_t nframes)
{

	/* This gets called from the engine server thread, so it must
	 * be serialized with the driver thread.  Stopping the audio
	 * also stops that thread. */
	 
	 
	closePandaAudioInstance(driver->stream);
	 
	driver->stream = openPandaAudioInstance((float)driver->frame_rate,driver->frames_per_cycle,driver->capturing,driver->playing,&driver->driver_name[0]);
	 
	if(!driver->stream) return FALSE;
	
	setHostData(driver->stream,driver);
	setCycleFun(driver->stream,coreaudio_runCycle);
	setParameter(driver->stream,'inte',&driver->isInterleaved);
	 
	driver->incoreaudio = getPandaAudioInputs(driver->stream);
	driver->outcoreaudio = getPandaAudioOutputs(driver->stream);
	 

	return startPandaAudioProcess(driver->stream);
}
#endif



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
				char* driver_name,
				AudioDeviceID deviceID)
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
	driver->frame_rate = rate;
	driver->capturing = capturing;
	driver->playing = playing;

	driver->attach = (JackDriverAttachFunction) coreaudio_driver_attach;
	driver->detach = (JackDriverDetachFunction) coreaudio_driver_detach;
	driver->read = (JackDriverReadFunction) coreaudio_driver_read;
	driver->write = (JackDriverReadFunction) coreaudio_driver_write;
	driver->null_cycle = (JackDriverNullCycleFunction) coreaudio_driver_null_cycle;
	//driver->bufsize = (JackDriverBufSizeFunction) coreaudio_driver_bufsize;
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
	
	driver->stream = openPandaAudioInstance((float)rate,frames_per_cycle,chan_in,chan_out,&deviceName[0]);
	if(!driver->stream) goto error;
	
	driver->client = client; 
	driver->period_usecs = (((float) driver->frames_per_cycle) / driver->frame_rate) * 1000000.0f;
	
	setHostData(driver->stream,driver);
	setCycleFun(driver->stream,coreaudio_runCycle);
	setParameter(driver->stream,'inte',&driver->isInterleaved);
	setParameter(driver->stream,'nstr',&driver->numberOfStreams);
	setParameter(driver->stream,'nstO',&driver->out_numberOfStreams);
	
	JCALog("There are %d input streams.\n",driver->numberOfStreams);
	JCALog("There are %d output streams.\n",driver->out_numberOfStreams);
	
	driver->channelsPerStream = (int*)malloc(sizeof(int)*driver->numberOfStreams);
	driver->out_channelsPerStream = (int*)malloc(sizeof(int)*driver->out_numberOfStreams);
	
	setParameter(driver->stream,'cstr',driver->channelsPerStream);
	setParameter(driver->stream,'cstO',driver->out_channelsPerStream);
	
	driver->incoreaudio = getPandaAudioInputs(driver->stream);
	driver->outcoreaudio = getPandaAudioOutputs(driver->stream);
	
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
	closePandaAudioInstance(driver->stream);
	free(driver->channelsPerStream);
	free(driver->out_channelsPerStream);
	free(driver);
}

static void coreaudio_usage (void) {
	fprintf (stderr, "\n"
    
        "coreaudio PCM driver args:\n"
        "    -r sample-rate (default: 44.1kHz)\n"
        "    -c chan (default: harware)\n"
		"    -ci input channels\n"
		"    -co output channels\n"
        "    -p frames-per-period (default: 128)\n"
        "    -D duplex, (default: yes)\n"
        "    -C capture, (default: duplex)\n"
        "    -P playback, (default: duplex)\n"
        "    -n audio device name\n"
		"    -I audio device ID\n"
        );
}


//== driver "plugin" interface =================================================

/* DRIVER "PLUGIN" INTERFACE */

const char driver_client_name[] = "coreaudio";


jack_driver_t *
driver_initialize (jack_client_t *client, int argc, char **argv)
{
	jack_nframes_t srate = 44100;
	jack_nframes_t frames_per_interrupt = 128;
	int capture = FALSE;
	int playback = FALSE;
	int chan_in = -1;
	int chan_out = -1;
	char* name = NULL;
	int i;
	AudioDeviceID deviceID = 0;
        
        for (i = 1; i < argc; i++) {
                 
		if (argv[i][0] == '-') {
                
			switch (argv[i][1]) {
                        
                            case 'n':
                                    name = (argv[i+1]);
                                    i++;
                                    break;
                        
                            case 'D':
                                    capture = TRUE;
                                    playback = TRUE;
                                    break;
									
							case 'c':
									switch (argv[i][2]) {
									
										case 'i':
											chan_in = atoi (argv[i+1]);
											break;
											
										case 'o':
											chan_out = atoi (argv[i+1]);
											break;
											
										default:
											chan_in = atoi (argv[i+1]);
											chan_out = chan_in;
											break;
									
									}
                                    i++;
                                    break;
                                    
							case 'C':
                                    capture = TRUE;
                                    break;
    
                            case 'P':
                                    playback = TRUE;
                                    break;
                                    
                            case 'r':
                                    srate = atoi (argv[i+1]);
                                    i++;
                                    break;
                                    
                            case 'p':
                                    frames_per_interrupt = atoi (argv[i+1]);
                                    i++;
                                    break;
							case 'I':
								deviceID = atoi(argv[i+1]);
								i++;
								break;
                                    
                            default:
                                    coreaudio_usage ();
                                    return NULL;
			}
		} else {
                        coreaudio_usage ();
			return NULL;
		}
	}

	/* duplex is the default */

	if (!capture && !playback) {
		capture = TRUE;
		playback = TRUE;
	}
        
	return coreaudio_driver_new ("coreaudio", client, frames_per_interrupt, srate, capture, playback, chan_in, chan_out,name,deviceID);
}


void
driver_finish (jack_driver_t *driver)
{
	coreaudio_driver_delete ((coreaudio_driver_t *) driver);
}


