/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

typedef struct __jpplugdata JPPData;

enum { 
	kJackIsOff,kJackIsOn
};

JPPData * JPP_alloc(char *pluginPath);

void JPP_dealloc(JPPData *x);

void JPP_open_editor(JPPData *x);

void JPP_jack_status_has_changed(JPPData *x,int new_jack_status);

void *JPP_get_window_id(JPPData *x);


