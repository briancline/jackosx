/*
Copyright © Grame 2005

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

Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
grame@rd.grame.fr

*/

#include "TJackSynchro.h"
#include <stdio.h>
#include <assert.h>

// Static variables declaration
#define PRINT(x) { printf x; fflush(stdout); }
#define DBUG(x)  /*PRINT(x) */

static long gMaxCount = 10;
static long gCount = 0;

static float time_beats_per_bar = 4.0;
static float time_beat_type = 4.0;
static double time_ticks_per_beat = 1920.0;
static double time_beats_per_minute = 120.0;

static volatile int time_reset = 1;		/* true when time values change */

static void timebase(jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg)
{
	double min;			/* minutes since frame 0 */
	long abs_tick;		/* ticks since frame 0 */
	long abs_beat;		/* beats since frame 0 */

	if (new_pos || time_reset) {

		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = time_beats_per_minute;

		time_reset = 0;		/* time change complete */

		/* Compute BBT info from frame number.  This is relatively
		 * simple here, but would become complex if we supported tempo
		 * or time signature changes at specific locations in the
		 * transport timeline. */

		min = pos->frame / ((double) pos->frame_rate * 60.0);
		abs_tick = min * pos->beats_per_minute * pos->ticks_per_beat;
		abs_beat = abs_tick / pos->ticks_per_beat;

		pos->bar = abs_beat / pos->beats_per_bar;
		pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
		pos->tick = abs_tick - (abs_beat * pos->ticks_per_beat);
		pos->bar_start_tick = pos->bar * pos->beats_per_bar * pos->ticks_per_beat;
		pos->bar++;		/* adjust start to bar 1 */

#if 0
		/* some debug code... */
		fprintf(stderr, "\nnew position: %" PRIu32 "\tBBT: %3"
			PRIi32 "|%" PRIi32 "|%04" PRIi32 "\n",
			pos->frame, pos->bar, pos->beat, pos->tick);
#endif

	} else {

		/* Compute BBT info based on previous period. */
		pos->tick +=
			nframes * pos->ticks_per_beat * pos->beats_per_minute
			/ (pos->frame_rate * 60);

		while (pos->tick >= pos->ticks_per_beat) {
			pos->tick -= pos->ticks_per_beat;
			if (++pos->beat > pos->beats_per_bar) {
				pos->beat = 1;
				++pos->bar;
				pos->bar_start_tick +=
					pos->beats_per_bar
					* pos->ticks_per_beat;
			}
		}
	}
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::Shutdown(void *arg)
{
    exit(1);
}

//--------------------------------------------------------------------------------------------
int TJackSynchro::SyncCallback(jack_transport_state_t state, jack_position_t* pos, void* arg)
{
    TJackSynchro* synchro = (TJackSynchro*)arg;
	
    switch (state) {
        case JackTransportStopped:
            printf("state: Stopped\n");
            break;
        case JackTransportRolling:
            printf ("state: Rolling\n");
            break;
        case JackTransportStarting:
            printf("state: Starting\n");
            break;
        default:
            printf("state: [unknown]");
    }

    if (state == JackTransportStarting) {
        printf("--------------------------------------------------------\n");
        printf("Transport : Starting %ld\n", pos->frame);
		
		if (pos->frame == 0)
            synchro->RcvStart();
        else
            synchro->RcvContinue();
			
		/*
		// For slow sync testing...
		printf("gCount %ld\n", gCount);
		
		if (gCount == gMaxCount) {
			gCount = 0;
			return true;
		} else {
			gCount++;
			return false;
		}
		*/
	}

    return true;
}

//________________________________________________________________________________________
int TJackSynchro::Process(jack_nframes_t nframes, void *arg)
{
    TJackSynchro* client = (TJackSynchro*)arg;
    jack_position_t current;
    jack_transport_state_t transport_state = jack_transport_query (client->fClient, &current);

    if ((transport_state == JackTransportStopped) && (client->fTransportState == JackTransportRolling)) {
        printf("--------------------------------------------------------\n");
        printf("Transport : Stopped\n");
        client->RcvStop();
    }

    client->fTransportState = transport_state;
	return 0;
}

//________________________________________________________________________________________
void TJackSynchro::NotifyProc(const MIDINotification *message, void *refCon)
{
    if (message->messageID == kMIDIMsgSetupChanged) {
		// Nothing yet
	}
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc1(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[0], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc2(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[1], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc3(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[2], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc4(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[3], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc5(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[3], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc6(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[3], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc7(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[3], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadMidiProc8(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    TJackSynchro* synchro = (TJackSynchro*)refCon;
    if (synchro->fThru)
        OSStatus err = MIDIReceived(synchro->fMidiInput[3], pktlist);
}

//--------------------------------------------------------------------------------------------
void TJackSynchro::ReadSyncProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;
    TJackSynchro* synchro = (TJackSynchro*)refCon;

    for (int i = 0; i < pktlist->numPackets; ++i) {

		if (packet->length == 1) {

            switch (packet->data[0]) {

                case 0xFA:
					synchro->StartSync();
                    break;

                case 0xFC:
					synchro->StopSync();
                    break;

                case 0xFB:
                    synchro->StartSync();
					break;

                case 0xF8:
                    break;

            }

            packet = MIDIPacketNext(packet);
        }
	}
}

//------------------------------------------------------------------------
TJackSynchro::TJackSynchro()
{
    fClient = NULL;
    fStatus = kIdle;
    fThru = false;
    // Up to MAX_MIDI_BUS MIDI busses
    fReadProc.push_back(ReadMidiProc1);
    fReadProc.push_back(ReadMidiProc2);
    fReadProc.push_back(ReadMidiProc3);
    fReadProc.push_back(ReadMidiProc4);
    fReadProc.push_back(ReadMidiProc5);
    fReadProc.push_back(ReadMidiProc6);
    fReadProc.push_back(ReadMidiProc7);
    fReadProc.push_back(ReadMidiProc8);
}

//------------------------------------------------------------------------
TJackSynchro::~TJackSynchro()
{}

//------------------------------------------------------------------------
bool TJackSynchro::Open()
{
    OSStatus err;

    if ((fClient = jack_client_new("JackSynchro")) == 0) {
        fprintf (stderr, "jack server not running?\n");
        return false;
    }

    jack_set_process_callback(fClient, Process, this);
    jack_set_sync_callback(fClient, SyncCallback, this);
    jack_on_shutdown(fClient, Shutdown, 0);
	
	if (jack_activate(fClient)) {
        fprintf (stderr, "cannot activate client");
        return false;
    }

    DBUG(("MIDIClientCreate \n"));
    err = MIDIClientCreate(CFSTR("JAS Sync"), NotifyProc, NULL, &fMidiClient);
    if (!fClient) {
        printf("Can not open Midi client\n");
        goto error;
    }

    err = MIDIDestinationCreate(fMidiClient, CFSTR("Jack Sync In"), ReadSyncProc, this, &fDestination);
    if (!fDestination) {
        printf("Can not open create destination \n");
        goto error;
    }
    DBUG(("MIDIDestinationCreate OK\n"));
	
	DBUG(("MIDISourceCreate \n"));
    err = MIDISourceCreate(fMidiClient, CFSTR("Jack Sync Out"), &fSource);
    if (!fSource) {
        printf("Can not open create source \n");
        goto error;
    }
    return true;

error:
    Close();
    return false;
}

//------------------------------------------------------------------------
bool TJackSynchro::AddMidiBus(int i)
{
    OSStatus err;
    MIDIEndpointRef input = NULL;
    MIDIEndpointRef output = NULL;
    char inputStr [64];
    char outputStr [64];

    // Up to MAX_MIDI_BUS busses
    if (i >= MAX_MIDI_BUS)
        return false;

    sprintf(outputStr, "Jack MIDI In%ld", i);
    sprintf(inputStr, "Jack MIDI Out%ld", i);

    CFStringRef coutputStr = CFStringCreateWithCString(NULL, outputStr, CFStringGetSystemEncoding());
    CFStringRef cinputStr = CFStringCreateWithCString(NULL, inputStr, CFStringGetSystemEncoding());

    err = MIDIDestinationCreate(fMidiClient, coutputStr, fReadProc[i - 1], this, &output);
    if (!output) {
        printf("Can not open create destination \n");
        goto error;
    }

    err = MIDISourceCreate(fMidiClient, cinputStr, &input);
    if (!input) {
        printf("Can not open create source \n");
        goto error;
    }

    fMidiInput.push_back(input);
    fMidiOuput.push_back(output);
    CFRelease(coutputStr);
    CFRelease(cinputStr);
    return true;

error:
    if (input)
        MIDIEndpointDispose(input);
    if (output)
        MIDIEndpointDispose(output);
    return false;
}

//------------------------------------------------------------------------
void TJackSynchro::RemoveAllBusses()
{
    vector<MIDIEndpointRef>::const_iterator it;

    for (it = fMidiInput.begin(); it != fMidiInput.end(); it++) {
        MIDIEndpointRef endpoint = *it;
        MIDIEndpointDispose(endpoint);
    }

    for (it = fMidiOuput.begin(); it != fMidiOuput.end(); it++) {
        MIDIEndpointRef endpoint = *it;
        MIDIEndpointDispose(endpoint);
    }

    fMidiInput.clear();
    fMidiOuput.clear();
}

//------------------------------------------------------------------------
void TJackSynchro::Close()
{
    RemoveAllBusses();
    if (fClient)
        jack_client_close(fClient);
    if (fDestination)
        MIDIEndpointDispose(fDestination);
    if (fSource)
        MIDIEndpointDispose(fSource);
    if (fMidiClient)
        MIDIClientDispose(fMidiClient);
}

//------------------------------------------------------------------------
void TJackSynchro::StartSync()
{
    assert(fClient);
    jack_transport_locate(fClient, 0);
    jack_transport_start(fClient);
}

//------------------------------------------------------------------------
void TJackSynchro::StopSync()
{
    assert(fClient);
    jack_transport_stop(fClient);
}

//------------------------------------------------------------------------
void TJackSynchro::ContSync()
{
    assert(fClient);
    jack_transport_start(fClient);
}

//------------------------------------------------------------------------
void TJackSynchro::SendEvent(Byte c)
{
    MIDIPacketList realtimePacketList;
    realtimePacketList.numPackets = 1;
    realtimePacketList.packet[0].timeStamp = 0;
    realtimePacketList.packet[0].length = 1;
    realtimePacketList.packet[0].data[0] = c;
    OSStatus err = MIDIReceived(fSource, &realtimePacketList);
}

//------------------------------------------------------------------------
void TJackSynchro::RcvStart()
{
    SendEvent(0xFA);
}

//------------------------------------------------------------------------
void TJackSynchro::RcvStop()
{
    SendEvent(0xFC);
}

//------------------------------------------------------------------------
void TJackSynchro::RcvContinue()
{
    SendEvent(0xFB);
}

//------------------------------------------------------------------------
void TJackSynchro::RcvClock()
{
    SendEvent(0xF8);
}

//------------------------------------------------------------------------
void TJackSynchro::Locate(int frames)
{
    jack_transport_locate(fClient, frames);
}

