/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Foundation/Foundation.h>
#include <Panda/panda.h>
#include <Panda/variables.h>

enum { 
	kJackIsOff,kJackIsOn
};

@interface JPPlugin : NSObject {
	NSString *pluginName;
	int plugin_ID;
	void *var;
	bool released;
}

- (BOOL) open:(NSString*)plugName;
- (void) close;
- (void) openEditor;
- (void) jackStatusHasChanged:(int)newStatus;
- (id) getWindow;

@end
