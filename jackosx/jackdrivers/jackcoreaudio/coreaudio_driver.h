/*
    Copyright � Grame, 2003.
	Copyright � Johnny Petrantoni, 2003.

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
    
	30-01-04, Johnny Petrantoni: first code of the coreaudio driver.
   
*/

#ifndef __jack_coreaudio_driver_h__
#define __jack_coreaudio_driver_h__

#define kVersion 001

#if defined(__APPLE__) && defined(__POWERPC__) 
#include <memops.h>
#else
#include "memops.h"
#endif

#include <jack/types.h>
#include <jack/hardware.h>
#include <jack/driver.h>
#include <jack/jack.h>
#include <jack/internal.h>
#include "AudioRenderBridge.h"

typedef struct {

        JACK_DRIVER_DECL

        struct _jack_engine *engine;

        jack_nframes_t    frame_rate;
        jack_nframes_t    frames_per_cycle;
        unsigned long     user_nperiods;
		int		  capturing;
		int		  playing;

        channel_t    playback_nchannels;
        channel_t    capture_nchannels;

        jack_client_t *client;
        JSList   *capture_ports;
        JSList   *playback_ports;

        float **incoreaudio;
		float **outcoreaudio;
		int isInterleaved;
		int numberOfStreams;
		int out_numberOfStreams;
		int *channelsPerStream,*out_channelsPerStream;
        char driver_name[256];
        void *   stream;

} coreaudio_driver_t;

#endif /* __jack_coreaudio_driver_h__ */
