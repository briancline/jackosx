/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "Jack_udp.h"
#include <unistd.h>

#define LOCSTR(s) NSLocalizedString(s,nil)

int process(jack_nframes_t nframes, void *arg) {
	Jack_udp *c = (Jack_udp*)arg;
	[c jackProcess:nframes];
	return 0;
}


@implementation Jack_udp

- (void) awakeFromNib {
	setSharedPointer("NetJack",(void*)self); //this is the way we use to make possible the communication with my_plugin.c code
	isOn = NO;
	char hostName[256];
	gethostname(&hostName[0],256);
	[ipaddress setStringValue:[NSString stringWithCString:&hostName[0]]];
}

- (void) jackProcess:(jack_nframes_t)nframes {
	switch(modeOfStream) {
		case 'send':
		{
			float *toSend[nports];
			int i;
			for(i=0;i<nports;i++) {
				toSend[i] = jack_port_get_buffer(ports[i],nframes);
			}
			send_audio(stream,nframes,toSend);
		}
		return;
		case 'recv':
		{
			float *toReceive[nports];
			int i;
			for(i=0;i<nports;i++) {
				toReceive[i] = jack_port_get_buffer(ports[i],nframes);
			}
			receive_audio(stream,nframes,toReceive);
		}
		return;
		case 'sere':
		{
			int i;
			float *toSend[nports];
			for(i=0;i<nports;i++) {
				toSend[i] = jack_port_get_buffer(ports[i],nframes);
			}
			send_audio(stream,nframes,toSend);
			
			float *toReceive[nports];
			for(i=0;i<nports;i++) {
				toReceive[i] = jack_port_get_buffer(re_ports[i],nframes);
			}
			receive_audio(restream,nframes,toReceive);
		}
		return;
	}
}

- (void) setInterfaceStatus:(int)newStatus {
	switch(newStatus) {
		case kStatusEnabled:
			//[buffersize setEnabled:YES];
			[channels setEnabled:YES];
			[ipaddress setEnabled:YES];
			[mode setEnabled:YES];
			[portn setEnabled:YES];
			break;
		case kStatusDisabled:
			//[buffersize setEnabled:NO];
			[channels setEnabled:NO];
			[ipaddress setEnabled:NO];
			[mode setEnabled:NO];
			[portn setEnabled:NO];
			break;
	}   
}

- (BOOL) createNewStream {
	nports = [channels intValue];
	char cliName[256];
	[[ipaddress stringValue] getCString:&cliName[0]];
	client = jack_client_new(&cliName[0]);
	if(!client) return NO;
	if(nports<=0) return NO;
	ports = (jack_port_t**)malloc(sizeof(jack_port_t*)*nports);
	if(modeOfStream=='sere') re_ports = (jack_port_t**)malloc(sizeof(jack_port_t*)*nports);
	int i;
	for(i=0;i<nports;i++) {
		char newName[256];
		if(modeOfStream == 'send') sprintf(&newName[0],"NetIn%d",i+1);
		else if(modeOfStream=='recv') sprintf(&newName[0],"NetOut%d",i+1);
		else {
			sprintf(&newName[0],"NetOut%d",i+1);
			re_ports[i] = jack_port_register(client,&newName[0],JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
		}
		if(modeOfStream!='sere') ports[i] = jack_port_register(client,&newName[0],JACK_DEFAULT_AUDIO_TYPE,modeOfStream == 'send' ? JackPortIsInput : JackPortIsOutput,0);
		else {
			sprintf(&newName[0],"NetIn%d",i+1);
			ports[i] = jack_port_register(client,&newName[0],JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
		}
	}
	jack_set_process_callback(client,process,self);
	
	[[ipaddress stringValue] getCString:&cliName[0]];
	
	if(modeOfStream!='sere') stream = createNewStream (16384,[channels intValue],[portn intValue],modeOfStream == 'send' ? &cliName[0] : NULL,modeOfStream);
	else {
		stream = createNewStream (16384,[channels intValue],[portn intValue],&cliName[0],'send');
		restream = createNewStream (16384,[channels intValue],[portn intValue],NULL,'recv');
	}
	
	
	if(!stream) {
		jack_client_close(client);
		free(ports);
		return NO;
	}
	
	startStream(stream);
	if(modeOfStream=='sere') startStream(restream);
	jack_activate(client);
	
	return YES;
}

- (IBAction)toggleOnOff:(id)sender {
	if(!isOn) {
		
		switch([mode indexOfSelectedItem]) {
			case 0:
				modeOfStream = 'send';
				break;
			case 1:
				modeOfStream = 'recv';
				break;
			case 2:
				modeOfStream = 'sere';
				break;
		}
		
		if(![self createNewStream]) {
			NSRunCriticalAlertPanel(LOCSTR(@"Cannot initialize NetJack:"),LOCSTR(@"Invalid configuration, check parameters."),LOCSTR(@"Ok"),nil,nil);
			return;
		}
		[on_off_button setTitle:LOCSTR(@"Stop Transmission")];
		[self setInterfaceStatus:kStatusDisabled];
		isOn = YES;
	} else if(isOn) {
		if(modeOfStream=='sere') stopStream(restream);
		stopStream(stream);
		jack_deactivate(client);
		if(modeOfStream=='sere') closeStream(restream);
		closeStream(stream);
		jack_client_close(client);
		free(ports);
		[on_off_button setTitle:LOCSTR(@"Start Transmission")];
		[self setInterfaceStatus:kStatusEnabled];
		isOn = NO;
	}
}

- (void) openEditor {
	[window makeKeyAndOrderFront:nil];
}

- (void) setPath:(NSString*)thePath {
	path = [[NSString stringWithString:thePath] retain];
}

- (void) release {
	[window orderOut:nil];
	if(path) [path release];
	if(isOn) {
		if(modeOfStream=='sere') stopStream(restream);
		stopStream(stream);
		jack_deactivate(client);
		if(modeOfStream=='sere') closeStream(restream);
		closeStream(stream);
		jack_client_close(client);
		free(ports);
	}
}

- (void *) getWindow {
	return (void*)window;
}

- (void) jackStatusChanged:(BOOL)JackIsOn {
	if(!JackIsOn) {
		if(isOn) [self toggleOnOff:nil];
		[self setInterfaceStatus:kStatusDisabled];
		[on_off_button setEnabled:NO];
	} else {
		[self setInterfaceStatus:kStatusEnabled];
		[on_off_button setEnabled:YES];
	}
}

@end
