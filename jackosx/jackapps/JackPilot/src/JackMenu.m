/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "JackMenu.h"
#import "JPPlugin.h"
#import "JackCon1.3.h"

OSStatus GetTotalChannels (AudioDeviceID device, UInt32	*channelCount, Boolean isInput) 
{
    OSStatus			err = noErr;
    UInt32				outSize;
    Boolean				outWritable;
    AudioBufferList		*bufferList = nil;
    short				i;

    *channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration,  &outSize, &outWritable);
    if (err == noErr)
    {
        bufferList = (AudioBufferList*)malloc(outSize);
        
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {								
            for (i = 0; i < bufferList->mNumberBuffers; i++) 
                *channelCount += bufferList->mBuffers[i].mNumberChannels;
        }
        free(bufferList);
    }
 
    return (err);
}

@implementation JackMenu

- (void)awakeFromNib {
	
	plugins_ids = [[NSMutableArray array] retain];
    
    BOOL needsPref = NO;
    if([Utility initPreference]) needsPref = YES;

    [self writeHomePath];
    BOOL needsDisplayPref = NO;
    int testJAL;
    testJAL = jackALLoad();
    if(testJAL==1) {
        [JALin setIntValue:getInCH()];
        [JALout setIntValue:getOutCH()];
        switch(getAutoC()) {
            case 0:
                [JALauto setState:NSOffState];
                break;
            case 1:
                [JALauto setState:NSOnState];
                break;
            default:
                [JALauto setState:NSOffState];
                break;
        }
        if(getDefInput())[defInput setState:NSOnState];
        else [defInput setState:NSOffState];
        if(getDefOutput())[defOutput setState:NSOnState];
        else [defOutput setState:NSOffState];
        if(getSysOut())[sysDefOut setState:NSOnState];
        else [sysDefOut setState:NSOffState];
		if(getVerboseLevel()>0) [verboseBox setState:NSOnState];
		else [verboseBox setState:NSOffState];
    } 
    
    int test;
    test = checkJack();
    if(test!=0) { 
		openJackClient();
		ottieniPorte();
		jackstat=1; writeStatus(1);
		[isonBut setStringValue:LOCSTR(@"Jack is On")];
        [self setupTimer]; 
		[startBut setTitle:LOCSTR(@"Stop Jack")];
		[toggleDock setTitle:LOCSTR(@"Stop Jack")];
		[bufferText setEnabled:NO];
		[channelsTest setEnabled:NO];
		[inputChannels setEnabled:NO];
		[driverBox setEnabled:NO];
		[interfaceBox setEnabled:NO];
		[samplerateText setEnabled:NO];
		[jackdMode setEnabled:NO];
    } else {
		[routingBut setEnabled:NO];
		jackstat=0; 
		writeStatus(0);
		[isonBut setStringValue:LOCSTR(@"Jack is Off")];
		[loadText setFloatValue:0.0f];
	}
	
	NSString *file;
	NSString *stringa = [NSString init];
	[driverBox removeAllItems];
	NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/usr/local/lib/jack/"];
	while (file = [enumerator nextObject]) {
		if ([file hasPrefix:@"jack_"]) {
				stringa = (NSString*)[file substringFromIndex:5];
				NSArray *listItems = [[stringa componentsSeparatedByString:@"."] retain];
				stringa = [listItems objectAtIndex:0];
				[driverBox addItemWithTitle:stringa];
				[listItems release];
		}
	}
	if([driverBox numberOfItems]>getDRIVER()) [driverBox selectItemAtIndex:getDRIVER()];
    
	if(needsPref) { 
		JPLog("Reading preferences\n");
		[self reloadPref:nil];
		NSArray *prefs = [Utility getPref:'audi'];
		if(prefs) {
			[driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
			[interfaceBox selectItemWithTitle:[prefs objectAtIndex:1]];
			[samplerateText selectItemWithTitle:[prefs objectAtIndex:2]];
			[bufferText selectItemWithTitle:[prefs objectAtIndex:3]];
			[channelsTest selectItemWithTitle:[prefs objectAtIndex:4]];
			[inputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
		}
	} else { needsDisplayPref=YES; /*[self jackALstore:nil];*/ }
    
	if(needsDisplayPref) { [prefWindow center]; [self openPrefWin:self]; }
	[self getJackInfo:self];
	[NSApp setDelegate:self];
	
	//restoring windows positions:
	NSArray *winPosPrefs = [Utility getPref:'winP'];
	if(winPosPrefs) {
		NSRect jpFrame;
		jpFrame.origin.x = [[winPosPrefs objectAtIndex:0] floatValue];
		jpFrame.origin.y = [[winPosPrefs objectAtIndex:1] floatValue];
		jpFrame.size.width = [[winPosPrefs objectAtIndex:2] floatValue];
		jpFrame.size.height = [[winPosPrefs objectAtIndex:3] floatValue];
		NSRect managerFrame;
		managerFrame.origin.x = [[winPosPrefs objectAtIndex:4] floatValue];
		managerFrame.origin.y = [[winPosPrefs objectAtIndex:5] floatValue];
		managerFrame.size.width = [[winPosPrefs objectAtIndex:6] floatValue];
		managerFrame.size.height = [[winPosPrefs objectAtIndex:7] floatValue];
		[jpWinController setFrame:jpFrame display:YES];
		[managerWin setFrame:managerFrame display:YES];
	}
	
	//Plugins-Menu stuff:
	#ifdef PLUGIN
	
	//NSString *fP_NSlots = [Utility getPref:'PlSL'];
	[self addPluginSlot];
	
	//__
	int i;
	//Plugins-restore latest status
	NSArray *pluginsToOpen = [Utility getPref:'PlOp'];
	for(i=0;i<[pluginsToOpen count];i++) {
		NSArray *plugArr = [pluginsToOpen objectAtIndex:i];
		NSString *plugName = [plugArr objectAtIndex:0];
		NSArray *windowOrder = [plugArr objectAtIndex:1];
		id slot = [pluginsMenu itemAtIndex:i];
		id slotMenu = [slot submenu];
		int nPlugs = [slotMenu numberOfItems];
		int a;
		for(a=0;a<nPlugs;a++) {
			id item = [slotMenu itemAtIndex:a];
			if([[item title] isEqualToString:plugName]) {
				id itemMenu = [item submenu];
				id openItem = [itemMenu itemWithTitle:LOCSTR(@"Open Instance")];
				[self openPlugin:openItem];
				id thePlug = [itemMenu delegate];
				id plugwindow = [thePlug getWindow];
				NSRect jpFrame;
				jpFrame.origin.x = [[windowOrder objectAtIndex:0] floatValue];
				jpFrame.origin.y = [[windowOrder objectAtIndex:1] floatValue];
				jpFrame.size.width = [[windowOrder objectAtIndex:2] floatValue];
				jpFrame.size.height = [[windowOrder objectAtIndex:3] floatValue];
			
				[plugwindow setFrame:jpFrame display:YES];
			}
		}
	}
	#endif
	//__
	
	[self reloadPref:nil];
    NSArray *prefs = [Utility getPref:'audi'];
    if(prefs) {
		BOOL needsRel = NO;
        [driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
		if(![[driverBox title] isEqualToString:[prefs objectAtIndex:0]]) [driverBox selectItemAtIndex:0];
		
        [interfaceBox selectItemWithTitle:[prefs objectAtIndex:1]];
		if(![[interfaceBox title] isEqualToString:[prefs objectAtIndex:1]]) [interfaceBox selectItemAtIndex:0];
		
        [samplerateText selectItemWithTitle:[prefs objectAtIndex:2]];
		if(![[samplerateText title] isEqualToString:[prefs objectAtIndex:2]]) [samplerateText selectItemAtIndex:0];
		
        [bufferText selectItemWithTitle:[prefs objectAtIndex:3]];
		if(![[bufferText title] isEqualToString:[prefs objectAtIndex:3]]) [bufferText selectItemAtIndex:0];
		
        [channelsTest selectItemWithTitle:[prefs objectAtIndex:4]];
		if(![[channelsTest title] isEqualToString:[prefs objectAtIndex:4]]) { needsRel = YES; [channelsTest selectItemAtIndex:0]; }
		
		[inputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
		if(![[inputChannels title] isEqualToString:[prefs objectAtIndex:5]]) { needsRel = YES; [inputChannels selectItemAtIndex:0]; }
		
		if(needsRel) [self reloadPref:nil];
    }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	NSMutableArray *toPrefs = [NSMutableArray array];
	NSRect jpFrame = [jpWinController frame];
	NSRect mangerFrame = [managerWin frame];
	
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.x]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.y]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.width]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.height]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.origin.x]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.origin.y]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.size.width]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.size.height]];
	
	if(![Utility savePref:toPrefs prefType:'winP'])NSLog(@"Cannot save windows positions");
	
	//Save plugins status
	#ifdef PLUGIN
	NSMutableArray *openedPlugs = [NSMutableArray array];
	
	int nSlots = [pluginsMenu numberOfItems]-1;
	int i;
	for(i=0;i<nSlots;i++) {
		id it = [pluginsMenu itemAtIndex:i];
		if([it state] == NSOnState) {
			id subMenu = [it submenu];
			int nItems = [subMenu numberOfItems];
			int a;
			for(a=0;a<nItems;a++) {
				id item = [subMenu itemAtIndex:a];
				if([item state]==NSOnState) {
					id plugInstance = [[item submenu] delegate];
					NSMutableArray *newPlugArr = [NSMutableArray array];
					[newPlugArr addObject:[item title]];
					id plugWindow = [plugInstance getWindow];
					
					NSMutableArray *toPrefs = [NSMutableArray array];
					NSRect jpFrame = [plugWindow frame];
		
					[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.x]];
					[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.y]];
					[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.width]];
					[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.height]];
					
					[newPlugArr addObject:toPrefs];
					
					[openedPlugs addObject:newPlugArr];
				}
			}
		}
	}	
	if(![Utility savePref:openedPlugs prefType:'PlOp'])NSLog(@"Cannot plugins instances");
	#endif
	//____
	return NSTerminateNow;
}

- (void)error:(NSString*)err 
{
    NSRunCriticalAlertPanel(LOCSTR(@"Error:"),err,@"Ok",nil,nil);
}

- (void)addPluginSlot {
	NSString *str = @"Slot ";
	NSNumber *actualN = [NSNumber numberWithInt:[pluginsMenu numberOfItems]+1];
	
	[pluginsMenu addItemWithTitle:[str stringByAppendingString:[actualN stringValue]] action:nil keyEquivalent:@""];
	
	NSMenu *thisSlot = [NSMenu alloc];
	[thisSlot initWithTitle:[str stringByAppendingString:[actualN stringValue]]];
	
	NSString *file = nil;
	id enumerator = nil;
    enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/Library/Application Support/JackPilot/Modules/"];
    while (file = [enumerator nextObject]) {
        if ([[file pathExtension] isEqualToString:@"jpmodule"] && ![file isEqualToString:@"PandaLuaSupport.jpmodule"]) {
                NSArray *listItems = [file componentsSeparatedByString:@"."];
                id stringa = [listItems objectAtIndex:0];
                [thisSlot addItemWithTitle:stringa action:nil keyEquivalent:@""];
				
				NSMenu *plugMenu = [NSMenu alloc];
				[plugMenu initWithTitle:stringa];

				
				id item = [NSMenuItem alloc];
				[item setTarget:self];
				[item initWithTitle:LOCSTR(@"Open Instance") action:@selector(openPlugin:) keyEquivalent:@""];
				[plugMenu addItem:item];
				
				id item2 = [NSMenuItem alloc];
				[item2 setTarget:self];
				[item2 initWithTitle:LOCSTR(@"Close Instance") action:nil keyEquivalent:@""];
				[plugMenu addItem:item2];
				[item2 setEnabled:NO];
				
				id item3 = [NSMenuItem alloc];
				[item3 setTarget:self];
				[item3 initWithTitle:LOCSTR(@"Open Editor") action:nil keyEquivalent:@""];
				[plugMenu addItem:item3];
				[item3 setEnabled:NO];
				
				[thisSlot setSubmenu:plugMenu forItem:[thisSlot itemWithTitle:stringa]];
				
        }
    }
	file = nil;
	enumerator = nil;
    enumerator = [[NSFileManager defaultManager] enumeratorAtPath:[NSHomeDirectory() stringByAppendingString:@"/Library/Application Support/JackPilot/Modules/"]];
    while (file = [enumerator nextObject]) {
        if ([[file pathExtension] isEqualToString:@"jpmodule"] && ![file isEqualToString:@"PandaLuaSupport.jpmodule"]) {
                NSArray *listItems = [file componentsSeparatedByString:@"."];
                id stringa = [listItems objectAtIndex:0];
                [thisSlot addItemWithTitle:stringa action:nil keyEquivalent:@""];
				
				NSMenu *plugMenu = [NSMenu alloc];
				[plugMenu initWithTitle:stringa];

				
				id item = [NSMenuItem alloc];
				[item setTarget:self];
				[item initWithTitle:LOCSTR(@"Open Instance") action:@selector(openPlugin:) keyEquivalent:@""];
				[plugMenu addItem:item];
				
				id item2 = [NSMenuItem alloc];
				[item2 setTarget:self];
				[item2 initWithTitle:LOCSTR(@"Close Instance") action:nil keyEquivalent:@""];
				[plugMenu addItem:item2];
				[item2 setEnabled:NO];
				
				id item3 = [NSMenuItem alloc];
				[item3 setTarget:self];
				[item3 initWithTitle:LOCSTR(@"Open Editor") action:nil keyEquivalent:@""];
				[plugMenu addItem:item3];
				[item3 setEnabled:NO];
				
				[thisSlot setSubmenu:plugMenu forItem:[thisSlot itemWithTitle:stringa]];
				
        }
    }
	
	[pluginsMenu setSubmenu:thisSlot forItem:[pluginsMenu itemWithTitle:[str stringByAppendingString:[actualN stringValue]]] ];
}

- (void) sendJackStatusToPlugins:(BOOL)isOn {
	int nplugs = [plugins_ids count];
	int i;
	for(i=0;i<nplugs;i++) {
		id plug = [plugins_ids objectAtIndex:i];
		if(plug) [plug jackStatusHasChanged:isOn?kJackIsOn:kJackIsOff];
	}
}

- (IBAction)startJack:(id)sender
{	
    if(jackdStartMode) [self launchJackCarbon:sender];
    else [self launchJackDeamon:sender];
    
    
    if([JALauto state]==NSOnState) {
        jackALStore([JALin intValue],[JALout intValue],1,
				[defInput state]==NSOnState ? TRUE : FALSE,
				[defOutput state]==NSOnState ? TRUE : FALSE,
				[sysDefOut state]==NSOnState ? TRUE : FALSE,
				[verboseBox state]==NSOnState ? 1 : 0,
				(int)selDevID);
    }
    if([JALauto state]==NSOffState) {
        jackALStore([JALin intValue],[JALout intValue],0,
		[defInput state]==NSOnState ? TRUE : FALSE,
		[defOutput state]==NSOnState ? TRUE : FALSE,
		[sysDefOut state]==NSOnState ? TRUE : FALSE,
		[verboseBox state]==NSOnState ? 1 : 0,
		(int)selDevID);
    }
    if(jackstat!=1) {
		
		goto end;
		
		NSString *str;
		str = [NSString stringWithCString:"/./CarbonJackd -R -d "];
		
		char *driver;
		driver = (char*)malloc(sizeof(char*)*[[driverBox stringValue] length]+2);
		[[driverBox titleOfSelectedItem] getCString:driver];
		char *samplerate;
		samplerate = (char*)malloc(sizeof(char*)*[[samplerateText stringValue] length]+2);
		[[samplerateText titleOfSelectedItem] getCString:samplerate];
		char *buffersize;
		buffersize = (char*)malloc(sizeof(char*)*[[bufferText stringValue] length]+2);
		[[bufferText titleOfSelectedItem] getCString:buffersize];
		char *channels;
		channels = (char*)malloc(sizeof(char*)*[[channelsTest stringValue] length]+2);
		[[channelsTest titleOfSelectedItem] getCString:channels];
		char *interface;
		NSString *interfaccia = [NSString init];
		interfaccia = [interfaceBox titleOfSelectedItem];
		interface = (char*)malloc(sizeof(char*)*[interfaccia length]+2);
		[interfaccia getCString:interface];
		
		char *stringa;
		stringa = (char*)malloc(sizeof(char*)*480);
		[jpPath getCString:stringa];
		
		strcat(stringa,"/./CarbonJackd -R -d ");
		strcat(stringa,driver);
		strcat(stringa," -r ");
		strcat(stringa,samplerate);
		strcat(stringa," -p ");
		strcat(stringa,buffersize);
		strcat(stringa," -c ");
		strcat(stringa,channels);
		strcat(stringa," -n ");
		strcat(stringa,"\"");
		strcat(stringa,interface);
		strcat(stringa,"\"");
		
		int a;
		id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is starting..."),nil,nil,nil);
		NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
		a = openJack(stringa);
		[NSApp endModalSession:modalSession];
		NSReleaseAlertPanel(pannelloDiAlert);

	end:
		{
		
		openJackClient();
		
		if(checkJack()!=0 && getClient()!=NULL){
			ottieniPorte();
			jackstat = 1;
			writeStatus(1); 
			[isonBut setStringValue:LOCSTR(@"Jack is On")];
			[startBut setTitle:LOCSTR(@"Stop Jack")]; [toggleDock setTitle:LOCSTR(@"Stop Jack")];
			[self setupTimer];
			[bufferText setEnabled:NO];
			[channelsTest setEnabled:NO];
			[inputChannels setEnabled:NO];
			[driverBox setEnabled:NO];
			[interfaceBox setEnabled:NO];
			[samplerateText setEnabled:NO];
			[jackdMode setEnabled:NO];
			[self sendJackStatusToPlugins:YES];
			[routingBut setEnabled:YES];
			[[JackConnections getSelf] JackCallBacks];
		}
		else { 
			jackstat = 0; 
			[self error:LOCSTR(@"Cannot start Jack server,\nPlease check the console or retry after a system reboot.")]; 
			writeStatus(0); 
		}
		//free(stringa); free(driver); free(samplerate); free(buffersize); free(channels); free(interface); 
		}
		[self jackALstore:sender];
    }
}

- (IBAction)stopJack:(id)sender
{
    if(jackstat==nil) { return; }
    if(jackstat==1) {
        [managerWin orderOut:sender];
        
        if(checkJack()!=0) {    
            id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is closing..."),nil,nil,nil);
            NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
            char command[256] = "osascript -e tell\\ application\\ \\\"jackd\\\"\\ to\\ quit";
            my_system2(&command[0]);
            int count = 0;
            while(checkJack()!=0) {
                sleep(1);
                count++;
                if(count==10) { 
                    [NSApp endModalSession:modalSession];
                    NSReleaseAlertPanel(pannelloDiAlert);
                    [self error:@"Jack server error, try manually (from Dock menu) to close jackd.."];
                    return;
                }
            }
            [NSApp endModalSession:modalSession];
            NSReleaseAlertPanel(pannelloDiAlert);
        }
        
        [self stopTimer];
        jackstat = 0; 
        writeStatus(0); 
        [isonBut setStringValue:LOCSTR(@"Jack is Off")];
        [loadText setFloatValue:0.0f];
        [cpuLoadBar setDoubleValue:0.0];
        [startBut setTitle:LOCSTR(@"Start Jack")];
        [toggleDock setTitle:LOCSTR(@"Start Jack")];
        [connectionsNumb setIntValue:0];
        [bufferText setEnabled:YES];
        [channelsTest setEnabled:YES];
		[inputChannels setEnabled:YES];
        [driverBox setEnabled:YES];
        [interfaceBox setEnabled:YES];
        [samplerateText setEnabled:YES];
        [jackdMode setEnabled:YES];
    }
}

-(void)warning:(id)sender
{
    int nClients = quantiClienti()-1;
	if([[channelsTest titleOfSelectedItem] intValue] == 0 && [[inputChannels titleOfSelectedItem] intValue] == 0) nClients++;
    int a;
	if(nClients>0) {
		NSString *youhave = LOCSTR(@"You have ");
		NSString *mess = LOCSTR(@" clients running, they will stop working or maybe crash!!");
		NSString *manyClien = [[NSNumber numberWithInt:nClients] stringValue];
		NSMutableString *message = [NSMutableString stringWithCapacity:2];
		[message appendString:youhave];
		[message appendString:manyClien];
		[message appendString:mess];
        a = NSRunAlertPanel(LOCSTR(@"Are you sure?"),message,LOCSTR(@"No"),LOCSTR(@"Yes"),nil);
    } else {
        a = NSRunAlertPanel(LOCSTR(@"Are you sure?"),LOCSTR(@"There are no clients running."),LOCSTR(@"No"),LOCSTR(@"Yes"),nil);
    }
    
    switch(a) {
        case 0:
            if(jackdStartMode) [self stopJack:sender];
            else [self closeJackDeamon:sender];
            break;
        case -1:
            return;
            break;
        case 1:
            return;
            break;
    }
}

- (IBAction)toggleJack:(id)sender 
{
    switch(jackstat) {
        case 1:
            [self warning:sender];
            break;
        case 0:
            [self startJack:sender];
            break;
        default:
            break;
    }
}

- (IBAction)toggle2Jack:(id)sender 
{
    [self toggleJack:sender];
}

-(void)cpuMeasure
{
    float load = cpuLoad();
    [loadText setFloatValue:load];
    [cpuLoadBar setDoubleValue:(double)load];
}

-(void) setupTimer
{
    update_timer = [[NSTimer scheduledTimerWithTimeInterval: 1.0 target: self selector: @selector(cpuMeasure) userInfo:nil repeats:YES] retain];		
    [[NSRunLoop currentRunLoop] addTimer: update_timer forMode: NSDefaultRunLoopMode];
}

-(void) stopTimer
{
    [update_timer invalidate];
    [update_timer release];
    update_timer= nil;
}

-(IBAction)jackALstore:(id)sender 
{
    if([JALauto state]==NSOnState) {
        if(jackALStore([JALin intValue],[JALout intValue],1,
						[defInput state]==NSOnState ? TRUE : FALSE,
						[defOutput state]==NSOnState ? TRUE : FALSE,
						[sysDefOut state]==NSOnState ? TRUE : FALSE,
						[verboseBox state]==NSOnState ? 1 : 0,
						(int)selDevID)==22) [self error:@"Cannot save JAS preferences."];
    }
    if([JALauto state]==NSOffState) {
        if(jackALStore([JALin intValue],[JALout intValue],0,
						[defInput state]==NSOnState ? TRUE : FALSE,
						[defOutput state]==NSOnState ? TRUE : FALSE,
						[sysDefOut state]==NSOnState ? TRUE : FALSE,
						[verboseBox state]==NSOnState ? 1 : 0,
						(int)selDevID)==22) [self error:@"Cannot save JAS preferences."];
    }
    
    NSMutableArray *toFile;
    
    toFile = [NSMutableArray array];
    [toFile addObject:[driverBox titleOfSelectedItem]];
    [toFile addObject:[interfaceBox titleOfSelectedItem]];
    [toFile addObject:[samplerateText titleOfSelectedItem]];
    [toFile addObject:[bufferText titleOfSelectedItem]];
    [toFile addObject:[channelsTest titleOfSelectedItem]];
	[toFile addObject:[inputChannels titleOfSelectedItem]];
    [Utility savePref:toFile prefType:'audi'];
    
    [self closePrefWin:sender];
}

- (IBAction)openDocsFile:(id)sender {

    char *commando,*res,*buf2;
    commando = (char*)calloc(256,sizeof(char*));
    res = (char*)calloc(256,sizeof(char*));
    buf2 = (char*)calloc(256,sizeof(char*));
    NSBundle *pacco = [NSBundle bundleForClass:[self class]];
    NSString *path = [pacco resourcePath];
    NSArray *lista = [path componentsSeparatedByString:@" "];
    int i;
    for(i=0;i<[lista count];i++) {
        id item = [lista objectAtIndex:i];
        [item getCString:buf2];
        strcat(res,buf2);
        if(i+1!=[lista count])strcat(res,"\\ ");
    }
    strcpy(commando,"open ");
    strcat(commando,res);
    switch([sender tag]) {
        case 0:
            strcat(commando,"/jackdoc.pdf");
            break;
        case 1:
            strcat(commando,"/jackdocIT.pdf");
            break;
        case 2:
            strcat(commando,"/jackdocFR.pdf");
            break;
        default:
            strcat(commando,"/jackdoc.pdf");
            break;
    }
    my_system2(commando);
    free(commando); free(res); free(buf2);
}

- (int)writeHomePath {
    NSString *homePath = NSHomeDirectory();
    char *thePath;
    thePath = (char*)alloca(256*sizeof(char));
    [homePath getCString:thePath];
    writeHomePath(thePath);
    return 1;
}

- (IBAction)startJackTask:(id)sender {
    NSMutableArray *arrArg = [NSMutableArray array];
    NSString *uno = @"-R";
    NSString *due = @"-d";
    NSString *tre = @"portaudio";
    NSString *quattro = @"-r";
    NSString *cinque = @"44100";
    NSString *sei = @"-p";
    NSString *sette = @"512";
    NSString *otto = @"-c";
    NSString *nove = @"2";
    NSString *dieci = @"-n";
    NSString *undici = @"\"Built-In Audio controller\"";
    [arrArg addObject:uno];
    [arrArg addObject:due];[arrArg addObject:tre];[arrArg addObject:quattro];[arrArg addObject:cinque];[arrArg addObject:sei];[arrArg addObject:sette];
    [arrArg addObject:otto];[arrArg addObject:nove];[arrArg addObject:dieci];[arrArg addObject:undici];
    id jackTask2 = [NSTask launchedTaskWithLaunchPath:@"/usr/local/bin/jackd" arguments:arrArg];
    [jackTask2 waitUntilExit];
    int status = [jackTask2 terminationStatus];
    
    if (status == 0)
        NSLog(@"Task succeeded.");
    else
        NSLog(@"Task failed.");
}

- (IBAction)getJackInfo:(id)sender {
    NSBundle *bundle = [NSBundle bundleWithIdentifier:@"com.grame.JackAudioServer"];
	if(!bundle) bundle = [NSBundle bundleWithIdentifier:@"com.grame.JackRouter"];
    if(!bundle) {
		NSLog(@"JAS not found");
		[jasVerText setStringValue:LOCSTR(@"not installed.")];
		[Utility error:'jasN'];
    } else {
        id dict = [bundle infoDictionary];
        jasVer = [dict objectForKey:@"CFBundleVersion"];
        jasCopyR = [dict objectForKey:@"CFBundleGetInfoString"];
        [jasVerText setStringValue:jasVer];
        [jasCopyRText setStringValue:jasCopyR];
    }
    
    bundle = [NSBundle bundleWithIdentifier:@"com.grame.Jack"];
    if(!bundle) {
        NSLog(@"JackFramework not found");
        [jackVerText setStringValue:LOCSTR(@"Maybe too old.")];
    } else {
        id dict = [bundle infoDictionary];
        jackVer = [dict objectForKey:@"CFBundleVersion"];
        jackCopyR = [dict objectForKey:@"CFBundleGetInfoString"];
        [jackVerText setStringValue:jackVer];
        [jackCopyRText setStringValue:jackCopyR];
    }
    
    bundle = [NSBundle bundleForClass:[self class]];
    if(!bundle) {
        NSLog(@"Bundle not found");
        [jpVerText setStringValue:LOCSTR(@"corrupted")];
    } else {
        id dict = [bundle infoDictionary];
        jpVer = [dict objectForKey:@"CFBundleVersion"];
        jpCopyR = [dict objectForKey:@"CFBundleGetInfoString"];
        [jpVerText setStringValue:jpVer];
        [jpCopyRText setStringValue:jpCopyR];
        jpPath = [bundle resourcePath];
    }
}

-(IBAction)openJackOsxNet:(id)sender {
    char *command = (char*)calloc(256,sizeof(char));
    strcpy(command,"open http://www.jackosx.com");
    my_system2(command);
    free(command);
}

-(IBAction)openJohnnyMail:(id)sender {
    char *command = (char*)calloc(256,sizeof(char));
    strcpy(command,"open mailto:johnny@meskalina.it");
    my_system2(command);
    free(command);
}

- (IBAction)openPrefWin:(id)sender {
    [self reloadPref:sender];
	if(sender) {
		NSArray *prefs = [Utility getPref:'audi'];
		if(prefs) {
			BOOL needsReload = NO;
			
			[driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
			if(![[driverBox title] isEqualToString:[prefs objectAtIndex:0]]) [driverBox selectItemAtIndex:0];
			
			[interfaceBox selectItemWithTitle:[prefs objectAtIndex:1]];
			if(![[interfaceBox title] isEqualToString:[prefs objectAtIndex:1]]) [interfaceBox selectItemAtIndex:0];
			
			[samplerateText selectItemWithTitle:[prefs objectAtIndex:2]];
			if(![[samplerateText title] isEqualToString:[prefs objectAtIndex:2]]) [samplerateText selectItemAtIndex:0];
			
			[bufferText selectItemWithTitle:[prefs objectAtIndex:3]];
			if(![[bufferText title] isEqualToString:[prefs objectAtIndex:3]]) [bufferText selectItemAtIndex:0];
			
			[channelsTest selectItemWithTitle:[prefs objectAtIndex:4]];
			if(![[channelsTest title] isEqualToString:[prefs objectAtIndex:4]]) { needsReload = YES; [channelsTest selectItemAtIndex:0]; }
			
			[inputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
			if(![[inputChannels title] isEqualToString:[prefs objectAtIndex:5]]) { needsReload = YES; [inputChannels selectItemAtIndex:0]; }
			
			if(needsReload) [self reloadPref:nil];
		}
	}
    
    [prefWindow center];
    [prefWindow makeKeyAndOrderFront:sender];
    
    //[prefWindow display];
}
- (IBAction)closePrefWin:(id)sender {
    [prefWindow orderOut:sender];
}

- (IBAction) openAboutWin:(id)sender {
    modalSex2 = [NSApp beginModalSessionForWindow:aboutWin];
}

-(IBAction) closeAboutWin:(id)sender {
    [NSApp endModalSession:modalSex2];
    [aboutWin orderOut:sender];
}

- (IBAction) launchJackDeamon:(id) sender {
        
    char *driver;
    driver = (char*)malloc(sizeof(char)*[[driverBox stringValue] length]+2);
    [[driverBox titleOfSelectedItem] getCString:driver];
    char *samplerate;
    samplerate = (char*)malloc(sizeof(char)*[[samplerateText stringValue] length]+2);
    [[samplerateText titleOfSelectedItem] getCString:samplerate];
    char *buffersize;
    buffersize = (char*)malloc(sizeof(char)*[[bufferText stringValue] length]+2);
    [[bufferText titleOfSelectedItem] getCString:buffersize];
    char *channels;
    channels = (char*)malloc(sizeof(char)*[[channelsTest stringValue] length]+2);
    [[channelsTest titleOfSelectedItem] getCString:channels];
	char *in_channels;
    in_channels = (char*)malloc(sizeof(char)*[[inputChannels stringValue] length]+2);
    [[inputChannels titleOfSelectedItem] getCString:in_channels];
    char *interface;
    NSString *interfaccia = [NSString init];
    interfaccia = [interfaceBox titleOfSelectedItem];
    interface = (char*)malloc(sizeof(char)*[interfaccia length]+2);
    [interfaccia getCString:interface];
	
	OSStatus err = noErr;
	UInt32 size;
	AudioDeviceID vDevice = 0;
	Boolean isWritable;

	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
	if (err!=noErr) return;

	int manyDevices = size/sizeof(AudioDeviceID);

	AudioDeviceID devices[manyDevices];
	err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices);
	if (err!=noErr) return;

	int i;

	for(i=0; i<manyDevices; i++) {
		size = sizeof(char)*256;
		char name[256];
		err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name);
		if(err!=noErr) return;
		if(strncmp(interface,name,strlen(interface))==0) {  
			vDevice = devices[i];
		}
	}

	NSString *deviceIDStr = [[NSNumber numberWithLong:vDevice] stringValue];
	[deviceIDStr getCString:interface];
		
	if(strcmp(channels,"0")==0 && strcmp(in_channels,"0")==0) {
		openJack("");
		free(driver); free(samplerate); free(buffersize); free(channels); free(in_channels); free(interface);
		return;
	}
	
    char *stringa;
    stringa = (char*)malloc(sizeof(char)*480);
	memset(stringa,0x0,sizeof(char)*480);
    
    strcpy(stringa,"/usr/local/bin/./jackd -R -d ");
    strcat(stringa,driver);
	strcat(stringa," -r ");
    strcat(stringa,samplerate);
    strcat(stringa," -p ");
    strcat(stringa,buffersize);
    strcat(stringa," -o ");
    strcat(stringa,channels);
	strcat(stringa," -i ");
    strcat(stringa,in_channels);
	strcat(stringa," -I ");
	strcat(stringa,interface);
    
    int a;
    id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is starting..."),nil,nil,nil);
    NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
    a = openJack(stringa);
    [NSApp endModalSession:modalSession];
    NSReleaseAlertPanel(pannelloDiAlert);
    
    free(stringa); free(driver); free(samplerate); free(buffersize); free(channels); free(in_channels); free(interface); 
}

- (IBAction) closeJackDeamon:(id) sender {
	[managerWin orderOut:sender];
	if(checkJack()!=0) {    
		[self sendJackStatusToPlugins:NO];
		id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is closing..."),nil,nil,nil);
		NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
		closeJack();
		
		sleep(4);
		[NSApp endModalSession:modalSession];
		NSReleaseAlertPanel(pannelloDiAlert);
	}
	
	[self stopTimer];
	jackstat = 0; 
	writeStatus(0); 
	[isonBut setStringValue:LOCSTR(@"Jack is Off")];
	[loadText setFloatValue:0.0f];
	[cpuLoadBar setDoubleValue:0.0];
	[startBut setTitle:LOCSTR(@"Start Jack")];
	[toggleDock setTitle:LOCSTR(@"Start Jack")];
	[connectionsNumb setIntValue:0];
	[bufferText setEnabled:YES];
	[channelsTest setEnabled:YES];
	[inputChannels setEnabled:YES];
	[driverBox setEnabled:YES];
	[interfaceBox setEnabled:YES];
	[samplerateText setEnabled:YES];
	[routingBut setEnabled:NO];
}

- (int) launchJackCarbon:(id) sender {
    
    NSArray *prefs = [Utility getPref:'audi'];
    if(prefs) {
        [driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
        [interfaceBox selectItemWithTitle:[prefs objectAtIndex:1]];
        [samplerateText selectItemWithTitle:[prefs objectAtIndex:2]];
        [bufferText selectItemWithTitle:[prefs objectAtIndex:3]];
        [channelsTest selectItemWithTitle:[prefs objectAtIndex:4]];
		[inputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
    } else [self openPrefWin:sender];

    if(prefs) {
		NSString *commandPath = [jpPath stringByAppendingString:@"/jackd.app/Contents/MacOS/jackd"];
		NSMutableArray *args = [NSMutableArray array];
		[args addObject:@"-F"];
		[args addObject:@"-R"];
		[args addObject:@"-d"];
		[args addObject:[prefs objectAtIndex:0]];
		[args addObject:@"-r"];
		[args addObject:[prefs objectAtIndex:2]];
		[args addObject:@"-p"];
		[args addObject:[prefs objectAtIndex:3]];
		[args addObject:@"-o"];
		[args addObject:[prefs objectAtIndex:4]];
		[args addObject:@"-i"];
		[args addObject:[prefs objectAtIndex:5]];
		[args addObject:@"-I"];
		
		char interfaceArr[60];
		char *interface = &interfaceArr[0];
		
		[[prefs objectAtIndex:1] getCString:interface];
		
		OSStatus err = noErr;
		UInt32 size;
	
		AudioDeviceID vDevice = 0;
	
		Boolean isWritable;
	
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
		if(err!=noErr) return 1;
    
		int manyDevices = size/sizeof(AudioDeviceID);
	
		AudioDeviceID devices[manyDevices];
		err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices);
		if(err!=noErr) return 1;
	 
		int i;
	
		for(i=0;i<manyDevices;i++) {
			size = sizeof(char)*256;
			char name[256];
			err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name);
			if(err!=noErr) return 1;
			if(strncmp(interface,name,strlen(interface))==0) {  
				vDevice = devices[i];
			}
		}

		[args addObject:[[NSNumber numberWithLong:vDevice] stringValue]];
    
		id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is starting..."),nil,nil,nil);
		NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
		jackTask = [[NSTask launchedTaskWithLaunchPath:commandPath arguments:args] retain];
		sleep(4);
		[jackTask release];
		[NSApp endModalSession:modalSession];
		NSReleaseAlertPanel(pannelloDiAlert);
    }
    return 0;
}

- (IBAction) reloadPref:(id) sender {
    if([interfaceBox titleOfSelectedItem]) [[interfaceBox titleOfSelectedItem] getCString:&selectedDevice[0]];
    [interfaceBox removeAllItems];
    [channelsTest removeAllItems];
	[inputChannels removeAllItems];
    [samplerateText removeAllItems];
    [bufferText removeAllItems];
    [bufferText addItemWithTitle:@"64"]; [bufferText addItemWithTitle:@"128"]; 
	[bufferText addItemWithTitle:@"256"]; [bufferText addItemWithTitle:@"512"]; 
	[bufferText addItemWithTitle:@"1024"]; [bufferText addItemWithTitle:@"2048"]; 
	[bufferText addItemWithTitle:@"4096"];
    
	[bufferText selectItemWithTitle:@"512"];
	
    if(![self writeDevNames]) {
        [Utility error:'aud2']; 
        [NSApp terminate:nil];
    }
    if(![self writeCAPref:sender]) {
        [Utility error:'aud2']; 
        [NSApp terminate:nil];
    }
}

- (BOOL) writeCAPref:(id)sender {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    int i;
    
    AudioDeviceID vDevice = selDevID; 
    
    UInt32 outChannels = 0;
    err = GetTotalChannels(vDevice,&outChannels,false);
    if(err!=noErr) { 
		NSLog(@"err in GetTotalChannels, set to 0");
			
		[channelsTest addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[channelsTest selectItemAtIndex:0];
	} else {
		int vOutChannels = (int)outChannels;
	
		[channelsTest addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[channelsTest selectItemAtIndex:0];
	
		for(i=0;i<vOutChannels;i++) {
			[channelsTest addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[channelsTest selectItemAtIndex:i+1];
		}
		JPLog("got output channels ok, %d channels\n",vOutChannels);
	}
	
	//why this!!!?? 
	/*
	if(vOutChannels<=0) {
		err = GetTotalChannels(vDevice,&outChannels,true);
		if(err!=noErr) { NSLog(@"err in GetTotalChannels"); }
		
		vOutChannels = (int)outChannels;
        
		for(i=0;i<vOutChannels;i++) {
			[channelsTest addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[channelsTest selectItemAtIndex:i+1];
		}
		
		JPLog("got input channels ok but output channels are 0, %d input channels\n",vOutChannels);
		JPLog("JackPilot will use input channels value\n");
	}
	*/
	
	UInt32 inChannels = 0;
	err = GetTotalChannels(vDevice,&inChannels,true);
    if(err!=noErr) { 
		NSLog(@"err in GetTotalChannels");
			
		[inputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[inputChannels selectItemAtIndex:0];
	} else {
		int vInChannels = (int)inChannels;
	
		[inputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[inputChannels selectItemAtIndex:0];
	
		for(i=0;i<vInChannels;i++) {
			[inputChannels addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[inputChannels selectItemAtIndex:i+1];
		}
		JPLog("got input channels ok, %d channels\n",vInChannels);
    }
	
    BOOL sampleRatesOk = YES;
    
    err = AudioDeviceGetPropertyInfo(vDevice,0,false,kAudioDevicePropertyStreamFormats,&size,&isWritable);
    if(err!=noErr) { NSLog(@"err in (info) kAudioDevicePropertyStreamFormats"); sampleRatesOk = NO; }
    
    if(sampleRatesOk) {
    
		int count = size/sizeof(AudioStreamBasicDescription);
    
		AudioStreamBasicDescription *SRs = (AudioStreamBasicDescription*)malloc(size);
		size = sizeof(AudioStreamBasicDescription)*count;
		err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormats,&size,SRs);
		if(err!=noErr) { NSLog(@"err in kAudioDevicePropertyStreamFormats"); sampleRatesOk = NO; }
    
		if(sampleRatesOk) {
			for(i=0;i<count;i++) {
				[samplerateText addItemWithTitle:[[NSNumber numberWithLong:(long)SRs[i].mSampleRate] stringValue]];
			}
		}

		JPLog("got samplerates ok\n");
  		free(SRs);
    }
   
    Float64 actualSr;
    size = sizeof(Float64);
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyNominalSampleRate,&size,&actualSr);
    if(err!=noErr) { 
		NSLog(@"err in kAudioDevicePropertyNominalSampleRate"); sampleRatesOk = NO; 
	}else if(!sampleRatesOk) { 
		[samplerateText addItemWithTitle:[[NSNumber numberWithLong:(long)actualSr] stringValue]]; sampleRatesOk = YES; 
	}else { 
		sampleRatesOk = YES; 
	}
    
    if(!sampleRatesOk) {
        size = sizeof(AudioStreamBasicDescription);
        AudioStreamBasicDescription monoDesc;
        err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyStreamFormat,&size,&monoDesc);
        if(err!=noErr) { sampleRatesOk = NO; }
        else {
            NSLog(@"writing sample rate using single stream description");
            actualSr = monoDesc.mSampleRate;
            sampleRatesOk = YES;
        }
    }
    
    if(!sampleRatesOk) { 
		NSLog(@"writing sample rate using a standard 44100 value"); 
		[samplerateText addItemWithTitle:[[NSNumber numberWithLong:44100L] stringValue]]; 
		[samplerateText selectItemWithTitle:[[NSNumber numberWithLong:44100L] stringValue]]; 
	}else 
		[samplerateText selectItemWithTitle:[[NSNumber numberWithLong:(long)actualSr] stringValue]];
    
    JPLog("got nominal samplerate ok\n");
    
    UInt32 newBufFrame;
    size = sizeof(UInt32);
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&newBufFrame);
    if(err!=noErr) return NO;
    
    //[bufferText selectItemWithTitle:[[NSNumber numberWithLong:newBufFrame] stringValue]];
	[bufferText selectItemWithTitle:@"512"]; // forcing to a lower value...
    
    JPLog("got actual buffersize ok\n");
    
    UInt32 theSize = sizeof(UInt32);
    UInt32 newBufSize = 64;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"64"]; }
    newBufSize = 128;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"128"]; }
    newBufSize = 256;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"256"]; }
    newBufSize = 512;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"512"]; }
    newBufSize = 1024;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"1024"]; }
    newBufSize = 2048;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"2048"]; }
    newBufSize = 4096;
    err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufSize);
    if(err!=noErr) { [bufferText removeItemWithTitle:@"4096"]; }

    JPLog("buffersizes tests ok\n");
	
	UInt32 oldSize = newBufFrame;
	size = sizeof(UInt32);
    err = AudioDeviceGetProperty(vDevice,0,false,kAudioDevicePropertyBufferFrameSize,&size,&newBufFrame);
    if(err!=noErr) return NO;
    
	if(oldSize!=newBufFrame) {
		err = AudioDeviceSetProperty(vDevice,NULL,0,false,kAudioDevicePropertyBufferFrameSize,theSize,&newBufFrame);
		if(err) { NSLog(@"err in kAudioDevicePropertyBufferFrameSize"); return NO; }
	}
    
    JPLog("set old buffersize ok\n");
    return YES;
}

- (BOOL)writeDevNames {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    AudioDeviceID defaultDev;
    int i;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,&size,&isWritable);
    if(err!=noErr) return NO;
    
    int manyDevices = size/sizeof(AudioDeviceID);
    
    AudioDeviceID devices[manyDevices];
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,&size,&devices[0]);
    if(err!=noErr) return NO;
        
    size = sizeof(AudioDeviceID);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,&size,&defaultDev);
    if(err!=noErr) return NO;
        
    BOOL selected = NO;
	
	JPLog("Selected device: %s.\n",selectedDevice);
    
    for(i=0;i<manyDevices;i++) {
        size = sizeof(char)*256;
        char name[256];
		memset(&name[0],0x0,sizeof(char)*256);
        err = AudioDeviceGetProperty(devices[i],0,false,kAudioDevicePropertyDeviceName,&size,&name[0]);
        if(err!=noErr) return NO;        
        if(strcmp(&name[0],"Jack Router")!=0 && strcmp(&name[0],"iSight")!=0) {
			JPLog("Adding device: %s.\n",name);
            NSString *s_name = [NSString stringWithCString:&name[0]];
            [interfaceBox addItemWithTitle:s_name];
            if(strcmp(&selectedDevice[0],&name[0])==0) { 
				selected = YES; 
				[interfaceBox selectItemWithTitle:s_name]; 
				selDevID = devices[i]; 
			}
        }
    }
        
    if(!selected) { 
		selDevID = defaultDev; 
	}
    
	jackdStartMode = NO;
    return YES;
}

- (IBAction) switchJackdMode:(id)sender {
    JPLog("switch\n");
    
    switch([jackdMode state]) {
        case NSOffState:
        {
            [jackdMode setTitle:@"carbon"];
            jackdStartMode = YES;
            break;
        }
        case NSOnState:
        {
            [jackdMode setTitle:@"deamon"];
            jackdStartMode = NO;
            break;
        }
        default:
            break;
    }
    NSArray *prefs = [NSArray arrayWithObject:[jackdMode title]];
    [Utility savePref:prefs prefType:'jacd'];
}

#ifdef PLUGIN
- (void)testPlugin:(id)sender {
	JPPlugin *newPlugTest = [JPPlugin alloc];
	if([newPlugTest open:@"JPPlugTest"]) {
		NSLog(@"Plugin opened, testing:");
		[newPlugTest openEditor];
		//[newPlugTest closeEditor];
		[newPlugTest jackStatusHasChanged:kJackIsOff];
		[newPlugTest jackStatusHasChanged:kJackIsOn];
		[newPlugTest release];
	}
}

- (void) openPlugin:(id)sender {
	JPPlugin *newPlug = [JPPlugin alloc];
	if([newPlug open:[[sender menu] title]]) {
		[[sender menu] setDelegate:newPlug];
		id item = [[sender menu] itemWithTitle:LOCSTR(@"Open Instance")];
		if(item) {
			[item setAction:nil];
			[ [[[sender menu] supermenu] itemWithTitle:[[sender menu] title]] setState:NSOnState];
		}
		id item2 = [[sender menu] itemWithTitle:LOCSTR(@"Close Instance")];
		[item2 setAction:@selector(closePlugin:)];
		
		id item3 = [[sender menu] itemWithTitle:LOCSTR(@"Open Editor")];
		[item3 setAction:@selector(openPluginEditor:)];
		
		[newPlug openEditor];
				
		id slotMenu = [[sender menu] supermenu];
		[slotMenu setAutoenablesItems:NO];
		
		int nItems = [slotMenu numberOfItems];
		int i;
		for(i=0;i<nItems;i++) {
			id it = [slotMenu itemAtIndex:i];
			if(![[it title] isEqualToString:[[sender menu] title]]) [it setEnabled:NO];
		}
		
		id upMenu = [slotMenu supermenu];
		
		nItems = [upMenu numberOfItems];
		for(i=0;i<nItems;i++) {
			id it = [upMenu itemAtIndex:i];
			if([[it title] isEqualToString:[slotMenu title]]) [it setState:NSOnState];
		}
		if(jackstat!=1) [newPlug jackStatusHasChanged:kJackIsOff];
		else [newPlug jackStatusHasChanged:kJackIsOn];
		
		[plugins_ids addObject:newPlug];
		[self addPluginSlot];
	} 
}

- (void) closePlugin:(id)sender {
	JPPlugin *plug = [[sender menu] delegate];
	
	//[plug release];
	[plugins_ids removeObject:plug];
	id item = [[sender menu] itemWithTitle:LOCSTR(@"Open Instance")];
	if(item) {
		[item setAction:@selector(openPlugin:)];
		[ [[[sender menu] supermenu] itemWithTitle:[[sender menu] title]] setState:NSOffState];
	}
	id item2 = [[sender menu] itemWithTitle:LOCSTR(@"Close Instance")];
	[item2 setAction:nil];
	id item3 = [[sender menu] itemWithTitle:LOCSTR(@"Open Editor")];
	[item3 setAction:nil];
	
	id slotMenu = [[sender menu] supermenu];
	[slotMenu setAutoenablesItems:YES];

	int nItems = [slotMenu numberOfItems];
	int i;
	for(i=0;i<nItems;i++) {
		id it = [slotMenu itemAtIndex:i];
		[it setEnabled:YES];
		id subItMenu = [it submenu];
		id itemToAble = [subItMenu itemWithTitle:LOCSTR(@"Open Instance")];
		[itemToAble setAction:@selector(openPlugin:)];
	}
	
	[slotMenu update];
	
	id upMenu = [slotMenu supermenu];
		
	nItems = [upMenu numberOfItems];
	for(i=0;i<nItems;i++) {
		id it = [upMenu itemAtIndex:i];
		if([[it title] isEqualToString:[slotMenu title]]) [it setState:NSOffState];
	}
	
	[pluginsMenu removeItem:[pluginsMenu itemAtIndex:[pluginsMenu numberOfItems]-1]];
}

- (void) openPluginEditor:(id)sender {
	JPPlugin *plug = [[sender menu] delegate];
	[plug openEditor];
}
#endif

@end
