/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#include "JPPlugin.h"
#include "Jack_udp.h"

#define LOCSTR(s) NSLocalizedString(s,nil)

struct __jpplugdata {
	int NoData;
	Jack_udp *my_class;
};

JPPData * JPP_alloc(char *pluginPath) {
	JPPData *x = (JPPData*)malloc(sizeof(JPPData));
	
	NSString *path = [NSString stringWithCString:pluginPath];
	
	if([NSBundle loadNibFile:[path stringByAppendingString:LOCSTR(@"/Contents/Resources/English.lproj/NetJack.nib")] externalNameTable:nil withZone:nil])NSLog(@"JPPlugin: nib loaded");
	else { NSLog(@"JPPlugin: nib not loaded"); return NULL; }
	
	x->my_class  = (Jack_udp*) sharedPointer("NetJack"); //this will get the Test object pointer
	removeSharedPointer("NetJack"); //the shared pointer has to be removed to make possible to use multiple instances of this plugin
	
	if(!x->my_class) { printf("JPPlugin: class not found\n"); return NULL; }
	
	[x->my_class setPath:path];
	
	return x;
}

void JPP_dealloc(JPPData *x) {
	[x->my_class release];
}

void JPP_open_editor(JPPData *x) {
	[x->my_class openEditor];
}

void JPP_jack_status_has_changed(JPPData *x,int new_jack_status) {
	[x->my_class jackStatusChanged:new_jack_status==kJackIsOn?YES:NO];
}

void *JPP_get_window_id(JPPData *x) {
	return [x->my_class getWindow];
}
