/* Jack_udp */
/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Cocoa/Cocoa.h>
#include <Jack/jack.h>
#include "NetPanda.h"

enum {
	kStatusEnabled,kStatusDisabled
};

@interface Jack_udp : NSObject
{
    //IBOutlet id buffersize;
    IBOutlet id channels;
    IBOutlet id ipaddress;
    IBOutlet id mode;
    IBOutlet id on_off_button;
    IBOutlet id portn;
    IBOutlet id window;
	NSString *path;
	id jackudp_task;
	BOOL isOn;
	UdpAudioStream stream;
	UdpAudioStream restream;
	jack_client_t *client;
	jack_port_t **ports;
	jack_port_t **re_ports;
	int modeOfStream;
	int nports;
}
- (BOOL) createNewStream;
- (IBAction)toggleOnOff:(id)sender;
- (void) openEditor;
- (void) setPath:(NSString*)thePath;
- (void *) getWindow;
- (void) jackProcess:(jack_nframes_t)nframes;
- (void) jackStatusChanged:(BOOL)JackIsOn;
- (void) setInterfaceStatus:(int)newStatus;
@end
