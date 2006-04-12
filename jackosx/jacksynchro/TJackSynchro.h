/*
  Copyright © Grame 2005

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
*/

#ifndef __TJackSynchro__
#define __TJackSynchro__

#include <string.h>
#include <vector>
#include <string>

#include <jack/jack.h>
#include <CoreMIDI/MIDIServices.h>

#define MAX_MIDI_BUS 8

using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif

    class TJackSynchro {

            enum {kMaster, kIdle, kSlave};

        private:

            jack_client_t * fClient;		// Jack client

            int fStatus;

            MIDIClientRef fMidiClient;        // CoreMidi client
            MIDIEndpointRef fDestination;     // Synchro ouput
            MIDIEndpointRef fSource;          // Synchro input

            vector<MIDIEndpointRef> fMidiInput;		// Vector of Midi input
            vector<MIDIEndpointRef> fMidiOuput;		// Vector of Midi output
            vector<MIDIReadProc> fReadProc;

            bool fThru;

            jack_transport_state_t fTransportState;

            void SendEvent(Byte c);

            static void NotifyProc(const MIDINotification* message, void* refCon);
            static void ReadSyncProc(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc1(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc2(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc3(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc4(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc5(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc6(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc7(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);
            static void ReadMidiProc8(const MIDIPacketList* pktlist, void* refCon, void* connRefCon);

            static void Shutdown (void*  arg);

            static int Process(jack_nframes_t nframes, void*  arg);
            static int SyncCallback(jack_transport_state_t state, jack_position_t *pos, void*  arg);

        public:

            TJackSynchro();
            virtual ~TJackSynchro();

            bool Open();
            void Close();

            void Locate(int frames);

            void RcvStart();
            void RcvStop();
            void RcvContinue();
            void RcvClock();

            void SendStart();
            void SendStop();
            void SendContinue();
            void SendClock();

            void StartSync();
            void StopSync();
            void ContSync();

            bool AddMidiBus(int i);
            void RemoveAllBusses();

            void SetThru(bool state)
            {
                fThru = state;
            }
            bool GetThru()
            {
                return fThru;
            }
    };


#ifdef __cplusplus
}
#endif


#endif
