/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "JPPlugin.h"

@implementation JPPlugin

- (BOOL) open:(NSString*)plugName {

	if(var) { DeleteVar(var); var = NULL; }
	
	char *newPath = "/Library/Application Support/JackPilot/Modules/";
	char *newExt = ".jpmodule";
	
	// this will change panda paths to JackPilot plugins path
	changeModulesSearchDirectory(newPath);
	changeModulesBundleExtension(newExt);
	
	char modName[60];
	[plugName getCString:&modName[0]];
	
	char modPath[256];
	
	strcpy(&modPath[0],newPath);
	strcat(&modPath[0],&modName[0]);
	strcat(&modPath[0],newExt);
	
	char *argv[1];
	argv[0] = &modPath[0];
	
	plugin_ID = OpenModule(&modName[0],1,&argv[0]);
	
	if(plugin_ID==-1) {
		//TRYING AS PLUA SCRIPT
		char *pl_args[1];
		pl_args[0] = &modPath[0];
		int PLua = OpenModule("PandaLuaSupport",1,pl_args);
		if(PLua != -1) plugin_ID = PLua;
	}
	
	if(plugin_ID==-1) {
		char homePath[256];
		[NSHomeDirectory() getCString:&homePath[0]];
		strcat(&homePath[0],"/Library/Application Support/JackPilot/Modules/");
		changeModulesSearchDirectory(&homePath[0]);
		changeModulesBundleExtension(newExt);
		
		strcpy(&modPath[0],&homePath[0]);
		strcat(&modPath[0],&modName[0]);
		strcat(&modPath[0],newExt);
	
		argv[0] = &modPath[0];
	
		plugin_ID = OpenModule(&modName[0],1,&argv[0]);
		
		if(plugin_ID==-1) {
			//TRYING AS PLUA SCRIPT
			char *pl_args[1];
			pl_args[0] = &modPath[0];
			int PLua = OpenModule("PandaLuaSupport",1,pl_args);
			if(PLua != -1) plugin_ID = PLua;
		}
	}
	
	if(plugin_ID==-1) goto error;
	
	if(!ModuleIsLoaded(plugin_ID)) goto error;
	
	var = NewVar(INT_T);
	
	released = NO;
	
	changeModulesSearchDirectory(NULL); // turning back to normal default panda settings
	changeModulesBundleExtension(NULL);
		
	return YES;
	
error:

	changeModulesSearchDirectory(NULL); // turning back to normal default panda settings
	changeModulesBundleExtension(NULL);
	
	return NO;
}

- (void) close {
	if(plugin_ID!=-1) CloseModule(plugin_ID);
	if(var) { DeleteVar(var); var = NULL; }
}

- (void) openEditor {
	ExecuteMethod(plugin_ID,0,NULL);
}

- (void) jackStatusHasChanged:(int)newStatus {
	setIntValue(var,newStatus);
	ExecuteMethod(plugin_ID,2,var);
}

- (id) getWindow {
	Var newVar;
	newVar.var_t = NULL;
	ExecuteMethod(plugin_ID,3,&newVar);
	if(newVar.var_t) return (id)newVar.var_t;
	return nil;
}

- (void) release {
	if(!released) { 
		[self close];
		released = YES;
	}
}

@end
