/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "JackMenu.h"
#import "JPPlugin.h"
#import "JackCon1.3.h"

#include <sys/sysctl.h>
#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreAudio/CoreAudio.h>

typedef	UInt8	CAAudioHardwareDeviceSectionID;

#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)

static bool gJackRunning = false;

static void JackInfoShutDown(int code, const char* reason, void *arg) 
{
	JPLog("JackInfoShutDown\n");
    
    if (gJackRunning) {
        id POOL = [[NSAutoreleasePool alloc] init];
        JackMenu* menu = (JackMenu*)arg;
        [menu closeJackDeamon1:0];
     
        NSString *mess1 = NSLocalizedString(@"Warning:", nil);
        NSString *mess2 = NSLocalizedString([NSString stringWithCString:reason], nil);
        NSString *mess3 = NSLocalizedString(@"Ok", nil);
        
        NSRunCriticalAlertPanel(mess1, mess2, mess3, nil, nil);
        [POOL release];
    }
}

static UInt32	sNumberCommonSampleRates = 15;
static Float64	sCommonSampleRates[] = {	  8000.0,  11025.0,  12000.0,
											 16000.0,  22050.0,  24000.0,
											 32000.0,  44100.0,  48000.0,
											 64000.0,  88200.0,  96000.0,
											128000.0, 176400.0, 192000.0 };

static bool IsRateCommon(Float64 inRate)
{
    UInt32 theIndex;
	bool theAnswer = false;
    
	for(theIndex = 0; !theAnswer && (theIndex < sNumberCommonSampleRates); ++theIndex)
	{
		theAnswer = inRate == sCommonSampleRates[theIndex];
	}
	return theAnswer;
}

static UInt32 GetNumberCommonRatesInRange(Float64 inMinimumRate, Float64 inMaximumRate)
{
	//	find the index of the first common rate greater than or equal to the minimum
	UInt32 theFirstIndex = 0;
	while ((theFirstIndex < sNumberCommonSampleRates) && (sCommonSampleRates[theFirstIndex] < inMinimumRate))
	{
		++theFirstIndex;
	}
	
	//	find the index of the first common rate greater than or equal to the maximum
	UInt32 theLastIndex = theFirstIndex;
	while ((theFirstIndex < sNumberCommonSampleRates) && (sCommonSampleRates[theLastIndex] < inMaximumRate))
	{
		++theLastIndex;
	}
	
	//	the number in the range is the difference
	UInt32 theAnswer = theLastIndex - theFirstIndex;
	if (IsRateCommon(inMinimumRate) || IsRateCommon(inMaximumRate))
	{
		++theAnswer;
	}
	return theAnswer;
}

static Float64 GetCommonSampleRateInRangeByIndex(Float64 inMinimumRate, Float64 inMaximumRate, UInt32 inIndex)
{
	Float64 theAnswer = 0.0;
	
	//	find the index of the first common rate greater than or equal to the minimum
	UInt32 theFirstIndex = 0;
	while((theFirstIndex < sNumberCommonSampleRates) && (sCommonSampleRates[theFirstIndex] < inMinimumRate))
	{
		++theFirstIndex;
	}
	
	//	find the index of the first common rate greater than or equal to the maximum
	UInt32 theLastIndex = theFirstIndex;
	while((theFirstIndex < sNumberCommonSampleRates) && (sCommonSampleRates[theLastIndex] < inMaximumRate))
	{
		++theLastIndex;
	}
	
	//	the number in the range is the difference
	UInt32 theNumberInRange = theLastIndex - theFirstIndex;
	if(IsRateCommon(inMinimumRate) || IsRateCommon(inMaximumRate))
	{
		++theNumberInRange;
	}
	
	//	get the value from the array if it's in range
	if(inIndex < theNumberInRange)
	{
		theAnswer = sCommonSampleRates[inIndex + theFirstIndex];
	}
	
	return theAnswer;
}

static int numberOfItemsInNominalSampleRateComboBox(AudioDeviceID device)
{
	int theAnswer = 0;
    OSStatus err;
    UInt32 size;
    int count;
    UInt32 theRangeIndex;
    Boolean isWritable;
	
	if (device != 0) {
	      
        err = AudioDeviceGetPropertyInfo(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &isWritable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
        
        count = size / sizeof(AudioValueRange);
        JPLog("Sample rate different values: %ld\n",count);
	
        AudioValueRange valueTable[count];
        err = AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, valueTable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
        
        AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &valueTable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
		
		//	iterate through the ranges and add the minimum, maximum, and common rates in between
		for (theRangeIndex = 0; theRangeIndex < count; ++theRangeIndex)
		{
			//	get the number of common rates in the rage
			UInt32 theNumberCommonRates = GetNumberCommonRatesInRange(valueTable[theRangeIndex].mMinimum, valueTable[theRangeIndex].mMaximum);
			
			//	count all the common rates in the range
			theAnswer += theNumberCommonRates;
			
			//	count the minimum if it isn't the first common rate
			if(!IsRateCommon(valueTable[theRangeIndex].mMinimum))
			{
				++theAnswer;
			}
			
			//	count the maximum if it isn't the last common rate
			if(!IsRateCommon(valueTable[theRangeIndex].mMaximum))
			{
				++theAnswer;
			}
		}
	}
	else
	{
		theAnswer = 1;
	}
    
	return theAnswer;
}

static Float64 objectValueForItemAtIndex(AudioDeviceID device, int inItemIndex)
{
	Float64 theAnswer = 0;
    OSStatus err;
    UInt32 size;
    int count;
    UInt32 theRangeIndex;
    Boolean isWritable;
		
	if (device != 0) {
        
        err = AudioDeviceGetPropertyInfo(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &isWritable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
        
        count = size / sizeof(AudioValueRange);
        JPLog("Sample rate different values: %ld\n",count);
	
        AudioValueRange valueTable[count];
        err = AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, valueTable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
        
        AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &valueTable);
        if (err != noErr) {
            NSLog(@"err in (info) kAudioDevicePropertyAvailableNominalSampleRates");
        }
	
		//	start counting at zero
		int theIndex = 0;
		
		//	iterate through the ranges and add the minimum, maximum, and common rates in between
		for (theRangeIndex = 0; (theAnswer == 0) && (theRangeIndex < count); ++theRangeIndex)
		{
			//	get the number of common rates in the rage
			int theNumberCommonRates = GetNumberCommonRatesInRange(valueTable[theRangeIndex].mMinimum, valueTable[theRangeIndex].mMaximum);
			
			//	get the first and last common rates in the range
			Float64 theFirstCommonRate = GetCommonSampleRateInRangeByIndex(valueTable[theRangeIndex].mMinimum, valueTable[theRangeIndex].mMaximum, 0);
			Float64 theLastCommonRate = GetCommonSampleRateInRangeByIndex(valueTable[theRangeIndex].mMinimum, valueTable[theRangeIndex].mMaximum, theNumberCommonRates - 1);
			
			//	it's the minimum, if the minimum isn't a common rate
			if(valueTable[theRangeIndex].mMinimum != theFirstCommonRate)
			{
				if(theIndex == inItemIndex)
				{
					theAnswer = valueTable[theRangeIndex].mMinimum;
				}
				else
				{
					++theIndex;
				}
			}
			
			//	check the common rates in the range
			if(theAnswer == 0)
			{
				if(inItemIndex < (theIndex + theNumberCommonRates))
				{
					//	inItemIndex is in the common rates between the range
					theAnswer = GetCommonSampleRateInRangeByIndex(valueTable[theRangeIndex].mMinimum, valueTable[theRangeIndex].mMaximum, inItemIndex - theIndex);
				}
				else if((inItemIndex == (theIndex + theNumberCommonRates)) && (valueTable[theRangeIndex].mMaximum != theLastCommonRate))
				{
					//	it's the maximum, since the maximum isn't a common rate
					theAnswer = valueTable[theRangeIndex].mMaximum;
				}
				
				//	increment by the number of common rates
				theIndex += theNumberCommonRates;
				
				//	also increment if the maximum isn't a common rate
				if(valueTable[theRangeIndex].mMaximum != theLastCommonRate)
				{
					++theIndex;
				}
			}
		}
	}
	else
	{
		theAnswer = 0;
	}
		
	return theAnswer;
}

static char* DefaultServerName()
{
    char* server_name;
    if ((server_name = getenv("JACK_DEFAULT_SERVER")) == NULL)
        server_name = "default";
    return server_name;
}

static void startCallback(CFNotificationCenterRef center,
                         void*	observer,
                         CFStringRef name,
                         const void* object,
                         CFDictionaryRef userInfo)
{}

static void stopCallback(CFNotificationCenterRef center,
                         void*	observer,
                         CFStringRef name,
                         const void* object,
                         CFDictionaryRef userInfo)
{
 	if (gJackRunning) {
        id POOL = [[NSAutoreleasePool alloc] init];
		gJackRunning = false;
	
		NSString *mess1 = NSLocalizedString(@"Fatal error:", nil);
		NSString *mess2 = NSLocalizedString(@"Jack server has been stopped, JackPilot will quit.", nil);
		NSString *mess3 = NSLocalizedString(@"Ok", nil);
	
		NSRunCriticalAlertPanel(mess1, mess2, mess3, nil, nil);
		closeJack();
        [POOL release];
		exit(1);
	}
}
/*
static void restartCallback(CFNotificationCenterRef center,
                         void*	observer,
                         CFStringRef name,
                         const void* object,
                         CFDictionaryRef userInfo)
{
	if (gJackRunning) {
		gJackRunning = false;
	
		NSString *mess1 = NSLocalizedString(@"Warning:", nil);
		NSString *mess2 = NSLocalizedString(@"CoreAudio driver has been restarted, unexpected behaviour may result.", nil);
		NSString *mess3 = NSLocalizedString(@"Ok", nil);
	
		NSRunCriticalAlertPanel(mess1, mess2, mess3, nil, nil);
		gJackRunning = true;
	}
}
 */

static void StartNotification()
{
	CFStringRef ref = CFStringCreateWithCString(NULL, DefaultServerName(), kCFStringEncodingMacRoman);
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
									NULL, startCallback, CFSTR("com.grame.jackserver.start"),
									ref, CFNotificationSuspensionBehaviorDeliverImmediately);
    
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
									NULL, stopCallback, CFSTR("com.grame.jackserver.stop"),
									ref, CFNotificationSuspensionBehaviorDeliverImmediately);
    /*
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
									NULL, restartCallback, CFSTR("com.grame.jackserver.restart"),
									ref, CFNotificationSuspensionBehaviorDeliverImmediately);
    */
	CFRelease(ref);
}

static void StopNotification()
{
	CFStringRef ref = CFStringCreateWithCString(NULL, DefaultServerName(), kCFStringEncodingMacRoman);
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
										CFSTR("com.grame.jackserver.start"), ref);
    
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
										CFSTR("com.grame.jackserver.stop"), ref);
    /*
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), NULL,
										CFSTR("com.grame.jackserver.restart"), ref);	
    */
	CFRelease(ref);
}

static OSStatus getTotalChannels(AudioDeviceID device, UInt32* channelCount, Boolean isInput);

static bool checkDevice(AudioDeviceID device)
{
    OSStatus err;
	
	// Output channels
    UInt32 outChannels = 0;
    err = getTotalChannels(device, &outChannels, false);
    if (err != noErr) { 
		NSLog(@"err in getTotalChannels");
		return false;
	} 
	
	// Input channels
	UInt32 inChannels = 0;
	err = getTotalChannels(device, &inChannels, true);
    if (err != noErr) { 
		NSLog(@"err in getTotalChannels");
		return false;
	} 
	
	JPLog("checkDevice input/output: input %ld output %ld \n", inChannels, outChannels);
	return (outChannels == 0) && (inChannels == 0) ? false : true;
}

static bool checkDeviceName(const char* deviceName)
{
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    int i;
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) 
		return false;
    
    int manyDevices = size/sizeof(AudioDeviceID);
	JPLog("number of audio devices: %ld\n", manyDevices);
    
    AudioDeviceID devices[manyDevices];
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) 
		return false;
 		 
    for (i = 0; i < manyDevices; i++) {
		char name[256];
		CFStringRef nameRef;
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
	    if (err != noErr) 
			return false;     
			
		CFStringGetCString(nameRef, name, 256, kCFStringEncodingMacRoman);
		CFRelease(nameRef);
		
        if (strcmp(name, deviceName) == 0)
			return checkDevice(devices[i]);
	}
        
    return false;
}

static OSStatus getDeviceUIDFromID(AudioDeviceID id, char* name])
{
    UInt32 size = sizeof(CFStringRef);
	CFStringRef UI;
    OSStatus res = AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceUID, &size, &UI);
	if (res == noErr) {
		CFStringGetCString(UI, name, 128, CFStringGetSystemEncoding());
		JPLog("getDeviceUIDFromID: name = %s\n", name);
		CFRelease(UI);
	} else {	
		JPLog("getDeviceUIDFromID: error\n");
	}
    return res;
}

static bool IsAggregateDevice(AudioDeviceID device)
{
    OSStatus err = noErr;
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);
    err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    
    if (err != noErr) {
        JPLog("Device does not have subdevices\n");
        return false;
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        JPLog("Device does has %d subdevices\n", num_devices);
        return true;
    }
}

static OSStatus getTotalChannels(AudioDeviceID device, UInt32* channelCount, Boolean isInput) 
{
    OSStatus			err = noErr;
    UInt32				outSize;
    Boolean				outWritable;
    AudioBufferList*	bufferList = 0;
	unsigned int i;
	
	*channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        bufferList = (AudioBufferList*)malloc(outSize);
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {								
            for (i = 0; i < bufferList->mNumberBuffers; i++) 
                *channelCount += bufferList->mBuffers[i].mNumberChannels;
        }
		if (bufferList) 
			free(bufferList);	
    }
	return (err);
}

static bool availableOneSamplerate(AudioDeviceID device, Float64 wantedSampleRate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    UInt32 used; 
    Float64 usedSampleRate;
     
    // Get running rate
    outSize = sizeof(UInt32);
    err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyDeviceIsRunningSomewhere, &outSize, &used);
    if (err != noErr) {
        JPLog("Cannot get device running state\n");
        return false;
    } 
     
    // Device is not used...
    if (used == 0)
        return true;
    
    // Get sample rate
    outSize =  sizeof(Float64);
    err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyNominalSampleRate, &outSize, &usedSampleRate);
    if (err != noErr) {
        JPLog("Cannot get current sample rate\n");
        return false;
    }
    
    return (wantedSampleRate == usedSampleRate);
}

static bool availableSamplerate(AudioDeviceID device, Float64 wantedSampleRate)
{
    OSStatus err = noErr;
    int i;
    
    AudioObjectID sub_device[32];
    UInt32 outSize = sizeof(sub_device);
    err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioAggregateDevicePropertyActiveSubDeviceList, &outSize, sub_device);
    
    if (err != noErr) {
        JPLog("Device does not have subdevices\n");
        return availableOneSamplerate(device, wantedSampleRate);
    } else {
        int num_devices = outSize / sizeof(AudioObjectID);
        JPLog("Device does has %d subdevices\n", num_devices);
        for (i = 0; i < num_devices; i++) {
            if (!availableOneSamplerate(sub_device[i], wantedSampleRate)) {
                return false;
            }
        }
        return true;
    }
}

@implementation JackMenu

- (void)awakeFromNib {
    
    id POOL = [[NSAutoreleasePool alloc] init];
	
	plugins_ids = [[NSMutableArray array] retain];
    
    BOOL needsPref = NO;
    if ([Utility initPreference]) 
		needsPref = YES;

    [self writeHomePath];
    int testJAL;
    testJAL = jackALLoad();
    if (testJAL == 1) {
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
        if (getDefInput())
			[defInput setState:NSOnState];
        else 
			[defInput setState:NSOffState];
            
        if (getDefOutput())
			[defOutput setState:NSOnState];
        else 
			[defOutput setState:NSOffState];
            
        if (getSysOut())
			[sysDefOut setState:NSOnState];
        else 
			[sysDefOut setState:NSOffState];
            
		if (getVerboseLevel() > 0)
			[verboseBox setState:NSOnState];
		else 
			[verboseBox setState:NSOffState];
            
        if (getHogMode() > 0)
			[hogBox setState:NSOnState];
		else 
			[hogBox setState:NSOffState];
    } 
    
    int test = checkJack();
    if (test != 0) { 
		openJackClient();
        gJackRunning = true;
        
        //[[JackConnections getSelf] JackCallBacks]; // not used
        jack_on_info_shutdown(getClient(), JackInfoShutDown, self);
        jack_activate(getClient());
        
		jackstat = 1; 
		writeStatus(1);
		[isonBut setStringValue:LOCSTR(@"Jack is On")];
		[self setupTimer]; 
		[startBut setTitle:LOCSTR(@"Stop Jack")];
		[toggleDock setTitle:LOCSTR(@"Stop Jack")];
		[bufferText setEnabled:NO];
		[outputChannels setEnabled:NO];
		[inputChannels setEnabled:NO];
		[driverBox setEnabled:NO];
		[interfaceInputBox setEnabled:NO];
        [interfaceOutputBox setEnabled:NO];
		[samplerateText setEnabled:NO];
		[jackdMode setEnabled:NO];
        [hogBox setEnabled:NO];
       
    } else {
		[routingBut setEnabled:NO];
		jackstat = 0; 
		writeStatus(0);
		[isonBut setStringValue:LOCSTR(@"Jack is Off")];
		[loadText setFloatValue:0.0f];
	}
	
	NSString *file;
	NSString *stringa = [NSString init];
	[driverBox removeAllItems];
    
    // Use jackdmp path
	// NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/usr/local/lib/jack/"];
	NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/usr/local/lib/jackmp/"];
	while (file = [enumerator nextObject]) {
        
        // Only display jack_coreaudio driver...
		// if ([file hasPrefix:@"jack_"]) {
        if ([file hasPrefix:@"jack_coreaudio"]) {
				stringa = (NSString*)[file substringFromIndex:5];
				NSArray *listItems = [[stringa componentsSeparatedByString:@"."] retain];
				stringa = [listItems objectAtIndex:0];
				[driverBox addItemWithTitle:stringa];
				[listItems release];
		}
	}
	if ([driverBox numberOfItems] > getDRIVER())
		[driverBox selectItemAtIndex:getDRIVER()];
    
	if (needsPref) { 
		JPLog("Reading preferences\n");
		[self reloadPref:nil];
		NSArray *prefs = [Utility getPref:'audi'];
		if (prefs) {
			JPLog("Reading preferences ref file OK\n");
			char deviceName[256];
			[[prefs objectAtIndex:1] getCString:deviceName];
			JPLog("Reading preferences ref file deviceName = %s\n", deviceName);
			if (checkDeviceName(deviceName)) { // Check is device kept in preference is available
				[driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
				[interfaceInputBox selectItemWithTitle:[prefs objectAtIndex:1]];
                [interfaceOutputBox selectItemWithTitle:[prefs objectAtIndex:2]];
         		[samplerateText selectItemWithTitle:[prefs objectAtIndex:3]];
				[bufferText selectItemWithTitle:[prefs objectAtIndex:4]];
				[outputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
				[inputChannels selectItemWithTitle:[prefs objectAtIndex:6]];
			}  else {  // Otherwise opens the preference dialog
				[prefWindow center]; 
				[self openPrefWin:self]; 
			}
		}
	} else { 
		[prefWindow center]; 
		[self openPrefWin:self]; 
	}
    
	[self getJackInfo:self];
	[NSApp setDelegate:self];
	
	// Restoring windows positions:
	NSArray *winPosPrefs = [Utility getPref:'winP'];
	if (winPosPrefs) {
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
	
	[self addPluginSlot];
	
	//__
	int i;
	//Plugins-restore latest status
	NSArray *pluginsToOpen = [Utility getPref:'PlOp'];
	for(i = 0; i < [pluginsToOpen count]; i++) {
		NSArray *plugArr = [pluginsToOpen objectAtIndex:i];
		NSString *plugName = [plugArr objectAtIndex:0];
		NSArray *windowOrder = [plugArr objectAtIndex:1];
		id slot = [pluginsMenu itemAtIndex:i];
		id slotMenu = [slot submenu];
		int nPlugs = [slotMenu numberOfItems];
		int a;
		for (a = 0; a < nPlugs; a++) {
			id item = [slotMenu itemAtIndex:a];
			if ([[item title] isEqualToString:plugName]) {
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
    if (prefs) {
    
		BOOL needsRel = NO;
        
        [driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
		if (![[driverBox title] isEqualToString:[prefs objectAtIndex:0]]) 
			[driverBox selectItemAtIndex:0];
		
        [interfaceInputBox selectItemWithTitle:[prefs objectAtIndex:1]];
		if (![[interfaceInputBox title] isEqualToString:[prefs objectAtIndex:1]]) 
			[interfaceInputBox selectItemAtIndex:0];
            
        [interfaceOutputBox selectItemWithTitle:[prefs objectAtIndex:2]];
		if (![[interfaceOutputBox title] isEqualToString:[prefs objectAtIndex:2]]) 
			[interfaceOutputBox selectItemAtIndex:0];
		
		[samplerateText selectItemWithTitle:[prefs objectAtIndex:3]];
		if(![[samplerateText title] isEqualToString:[prefs objectAtIndex:3]]) 
			[samplerateText selectItemAtIndex:0];
		
		[bufferText selectItemWithTitle:[prefs objectAtIndex:4]];
		if (![[bufferText title] isEqualToString:[prefs objectAtIndex:4]])
			[bufferText selectItemAtIndex:0];
		
		[outputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
		if (![[outputChannels title] isEqualToString:[prefs objectAtIndex:5]]) { 
			needsRel = YES; 
			[outputChannels selectItemAtIndex:0]; 
		}
		
		[inputChannels selectItemWithTitle:[prefs objectAtIndex:6]];
		if (![[inputChannels title] isEqualToString:[prefs objectAtIndex:6]]) {
			needsRel = YES; 
			[inputChannels selectItemAtIndex:0]; 
		}
		
		if (needsRel) 
			[self reloadPref:nil];
    }
    
    [POOL release];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
	NSMutableArray *toPrefs = [NSMutableArray array];
	NSRect jpFrame = [jpWinController frame];
	NSRect mangerFrame = [managerWin frame];
    
    // Close jack client
    closeJack1();
	
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.x]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.origin.y]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.width]];
	[toPrefs addObject:[NSNumber numberWithFloat:jpFrame.size.height]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.origin.x]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.origin.y]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.size.width]];
	[toPrefs addObject:[NSNumber numberWithFloat:mangerFrame.size.height]];
	
	if(![Utility savePref:toPrefs prefType:'winP'])NSLog(@"Cannot save windows positions");
	
	// Save plugins status
	#ifdef PLUGIN
	NSMutableArray *openedPlugs = [NSMutableArray array];
	
	int nSlots = [pluginsMenu numberOfItems]-1;
	int i;
	for (i = 0; i < nSlots; i++) {
		id it = [pluginsMenu itemAtIndex:i];
		if([it state] == NSOnState) {
			id subMenu = [it submenu];
			int nItems = [subMenu numberOfItems];
			int a;
			for (a = 0; a < nItems; a++) {
				id item = [subMenu itemAtIndex:a];
				if ([item state] == NSOnState) {
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
	if (![Utility savePref:openedPlugs prefType:'PlOp'])
		NSLog(@"Cannot plugins instances");
	#endif
    
    [POOL release];
	return NSTerminateNow;
}

- (void)error:(NSString*)err 
{
    NSRunCriticalAlertPanel(LOCSTR(@"Error:"),err,@"Ok",nil,nil);
}

- (void)addPluginSlot {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
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
    [POOL release];
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
    id POOL = [[NSAutoreleasePool alloc] init];
    
	if (![self launchJackDeamon:sender])
        return;
    
	char driverInputname[128];
	getDeviceUIDFromID(selInputDevID, driverInputname);
    
    char driverOutputname[128];
	getDeviceUIDFromID(selOutputDevID, driverOutputname);
    
    if ([JALauto state] == NSOnState) {
        jackALStore([JALin intValue],[JALout intValue],1,
					[defInput state] == NSOnState ? TRUE : FALSE,
					[defOutput state] == NSOnState ? TRUE : FALSE,
					[sysDefOut state] == NSOnState ? TRUE : FALSE,
					[verboseBox state] == NSOnState ? 1 : 0,
					driverInputname,
                    driverOutputname,
                    [hogBox state] == NSOnState ? 1 : 0);
    }
	
    if ([JALauto state] == NSOffState) {
		jackALStore([JALin intValue],[JALout intValue],0,
					[defInput state] == NSOnState ? TRUE : FALSE,
					[defOutput state] == NSOnState ? TRUE : FALSE,
					[sysDefOut state] == NSOnState ? TRUE : FALSE,
					[verboseBox state] == NSOnState ? 1 : 0,
					driverInputname,
                    driverOutputname,
                    [hogBox state] == NSOnState ? 1 : 0);
    }	
	
    openJackClient();
    gJackRunning = true;
    
    if (checkJack() != 0 && getClient() != NULL){
        jackstat = 1;
        writeStatus(1); 
        
        //[[JackConnections getSelf] JackCallBacks]; // not used
        jack_on_info_shutdown(getClient(), JackInfoShutDown, self);
        jack_activate(getClient());
        
        [isonBut setStringValue:LOCSTR(@"Jack is On")];
        [startBut setTitle:LOCSTR(@"Stop Jack")]; 
        [toggleDock setTitle:LOCSTR(@"Stop Jack")];
        [self setupTimer];
        [bufferText setEnabled:NO];
        [outputChannels setEnabled:NO];
        [inputChannels setEnabled:NO];
        [driverBox setEnabled:NO];
        [interfaceInputBox setEnabled:NO];
        [interfaceOutputBox setEnabled:NO];
        [samplerateText setEnabled:NO];
        [jackdMode setEnabled:NO];
        [hogBox setEnabled:NO];
        [self sendJackStatusToPlugins:YES];
        [routingBut setEnabled:YES];
      
    } else { 
        jackstat = 0; 
        [self error:LOCSTR(@"Cannot start Jack server,\nPlease check the console or retry after a system reboot.")]; 
        writeStatus(0); 
    }
    [self jackALstore:sender];
    
    [POOL release];
 }

-(void)warning:(id)sender
{
    id POOL = [[NSAutoreleasePool alloc] init];
    
    int nClients = quantiClienti()-1;
	if ([[outputChannels titleOfSelectedItem] intValue] == 0 && [[inputChannels titleOfSelectedItem] intValue] == 0) 
		nClients++;
    int a;
	if (nClients > 0) {
		NSString *youhave = LOCSTR(@"You have ");
		NSString *mess = LOCSTR(@" clients running, they will stop working or maybe crash!!");
		NSString *manyClien = [[NSNumber numberWithInt:nClients] stringValue];
		NSMutableString *message = [NSMutableString stringWithCapacity:2];
		[message appendString:youhave];
		[message appendString:manyClien];
		[message appendString:mess];
        a = NSRunAlertPanel(LOCSTR(@"Are you sure?"),message,LOCSTR(@"No"),LOCSTR(@"Yes"),nil);
    } else {
		a = 0;
    }
    
    switch(a) {
        case 0:
			[self closeJackDeamon:sender];
            break;
        case -1:
            break;
        case 1:
            break;
    }
    
    [POOL release];
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
    id POOL = [[NSAutoreleasePool alloc] init];
     
	char driverInputname[128];
	getDeviceUIDFromID(selInputDevID, driverInputname);
    
    char driverOutputname[128];
	getDeviceUIDFromID(selOutputDevID, driverOutputname);
 	
    if ([JALauto state] == NSOnState) {
        if (jackALStore([JALin intValue],[JALout intValue],1,
						[defInput state] == NSOnState ? TRUE : FALSE,
						[defOutput state] == NSOnState ? TRUE : FALSE,
						[sysDefOut state] == NSOnState ? TRUE : FALSE,
						[verboseBox state] == NSOnState ? 1 : 0,
						driverInputname,
                        driverOutputname,
                        [hogBox state] == NSOnState ? 1 : 0) == 0) 
            [self error:@"Cannot save JAS preferences."];
    }
	
    if ([JALauto state] == NSOffState) {
        if (jackALStore([JALin intValue],[JALout intValue],0,
						[defInput state] == NSOnState ? TRUE : FALSE,
						[defOutput state] == NSOnState ? TRUE : FALSE,
						[sysDefOut state] == NSOnState ? TRUE : FALSE,
						[verboseBox state] == NSOnState ? 1 : 0,
						driverInputname,
                        driverOutputname,
                        [hogBox state] == NSOnState ? 1 : 0) == 0)  
            [self error:@"Cannot save JAS preferences."];
    }
	
	// Save the preferences in a format jack clients can use when using the jack_client_open API
	char filename[255];
	snprintf(filename, 255, "%s/.jackdrc", getenv("HOME"));
	FILE* file = fopen(filename,"w");
    
    if (file) {
    
        fprintf(file,"/usr/local/bin/jackd \n");
        fprintf(file,"-R \n");
        fprintf(file, "-d %s \n",[[driverBox titleOfSelectedItem]cString]);
        fprintf(file, "-p %s \n",[[bufferText titleOfSelectedItem]cString]);
        fprintf(file, "-r %s \n",[[samplerateText titleOfSelectedItem]cString]);
        fprintf(file, "-i %s \n",[[inputChannels titleOfSelectedItem]cString]);
        fprintf(file, "-o %s \n",[[outputChannels titleOfSelectedItem]cString]);
        
        if (strcmp(driverInputname, driverOutputname) == 0) {
            fprintf(file, "-d %s \n",driverInputname); 
        } else {
            fprintf(file, "-C %s \n",driverInputname); 
            fprintf(file, "-P %s \n",driverInputname); 
        }
        
        fclose(file);
	}
   
    NSMutableArray *toFile;
    
    toFile = [NSMutableArray array];
    [toFile addObject:[driverBox titleOfSelectedItem]];
    [toFile addObject:[interfaceInputBox titleOfSelectedItem]];
    [toFile addObject:[interfaceOutputBox titleOfSelectedItem]];
    [toFile addObject:[samplerateText titleOfSelectedItem]];
    [toFile addObject:[bufferText titleOfSelectedItem]];
    [toFile addObject:[outputChannels titleOfSelectedItem]];
	[toFile addObject:[inputChannels titleOfSelectedItem]];
    [Utility savePref:toFile prefType:'audi'];
    
    [self closePrefWin:sender];
    [POOL release];
}

- (IBAction)openDocsFile:(id)sender {

    id POOL = [[NSAutoreleasePool alloc] init];
    
    char *commando,*res,*buf2;
    commando = (char*)calloc(256,sizeof(char*));
    res = (char*)calloc(256,sizeof(char*));
    buf2 = (char*)calloc(256,sizeof(char*));
    NSBundle *pacco = [NSBundle bundleForClass:[self class]];
    NSString *path = [pacco resourcePath];
    NSArray *lista = [path componentsSeparatedByString:@" "];
    int i;
    for (i = 0;i < [lista count]; i++) {
        id item = [lista objectAtIndex:i];
        [item getCString:buf2];
        strcat(res,buf2);
        if (i+1 != [lista count])
			strcat(res,"\\ ");
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
    free(commando); 
    free(res); 
    free(buf2);
    [POOL release];
}

- (int)writeHomePath {
    NSString *homePath = NSHomeDirectory();
    char *thePath;
    thePath = (char*)alloca(256*sizeof(char));
    [homePath getCString:thePath];
    writeHomePath(thePath);
    return 1;
}

- (IBAction)getJackInfo:(id)sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
    NSBundle *bundle = [NSBundle bundleWithIdentifier:@"com.grame.JackRouter"];
    if (!bundle) {
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
    
	bundle = [NSBundle bundleWithIdentifier:@"com.grame.Jackmp"];
    if (!bundle) {
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
    if (!bundle) {
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
    
    [POOL release];
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

-(IBAction)openAMS:(id)sender {
    char *command = (char*)calloc(256,sizeof(char));
    strcpy(command,"open \"/Applications/Utilities/Audio\ MIDI\ Setup.app\"");
    my_system2(command);
    free(command);
}

-(IBAction)openSP:(id)sender {
    char *command = (char*)calloc(256,sizeof(char));
    strcpy(command,"open \"/System/Library/PreferencePanes/Sound.prefPane\"");
    my_system2(command);
    free(command);
}

- (IBAction)openPrefWin:(id)sender {
    [self reloadPref:sender];
	if (sender) {
		NSArray *prefs = [Utility getPref:'audi'];
		if (prefs) {
			BOOL needsReload = NO;
			
			[driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
			if (![[driverBox title] isEqualToString:[prefs objectAtIndex:0]]) 
				[driverBox selectItemAtIndex:0];
			
			[interfaceInputBox selectItemWithTitle:[prefs objectAtIndex:1]];
			if (![[interfaceInputBox title] isEqualToString:[prefs objectAtIndex:1]]) 
				[interfaceInputBox selectItemAtIndex:0];
			
			[samplerateText selectItemWithTitle:[prefs objectAtIndex:2]];
			if (![[samplerateText title] isEqualToString:[prefs objectAtIndex:2]]) 
				[samplerateText selectItemAtIndex:0];
			
			[bufferText selectItemWithTitle:[prefs objectAtIndex:3]];
			if (![[bufferText title] isEqualToString:[prefs objectAtIndex:3]]) 
				[bufferText selectItemAtIndex:0];
			
			[outputChannels selectItemWithTitle:[prefs objectAtIndex:4]];
			if (![[outputChannels title] isEqualToString:[prefs objectAtIndex:4]]) { 
				needsReload = YES; 
				[outputChannels selectItemAtIndex:0]; 
			}
			
			[inputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
			if (![[inputChannels title] isEqualToString:[prefs objectAtIndex:5]]) { 
				needsReload = YES; 
				[inputChannels selectItemAtIndex:0]; 
			}
			
			if (needsReload) 
				[self reloadPref:nil];
		}
	}
    
    [prefWindow center];
    [prefWindow makeKeyAndOrderFront:sender];
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

- (void)closeFromCallback {
    [self closeJackDeamon:0];
}

- (bool) launchJackDeamon:(id) sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
        
    char* driver = (char*)malloc(sizeof(char)*[[driverBox stringValue] length]+2);
    [[driverBox titleOfSelectedItem] getCString:driver];
    char* samplerate = (char*)malloc(sizeof(char)*[[samplerateText stringValue] length]+2);
    [[samplerateText titleOfSelectedItem] getCString:samplerate];
    char* buffersize = (char*)malloc(sizeof(char)*[[bufferText stringValue] length]+2);
    [[bufferText titleOfSelectedItem] getCString:buffersize];
    char* out_channels = (char*)malloc(sizeof(char)*[[outputChannels stringValue] length]+2);
    [[outputChannels titleOfSelectedItem] getCString:out_channels];
	char* in_channels = (char*)malloc(sizeof(char)*[[inputChannels stringValue] length]+2);
    [[inputChannels titleOfSelectedItem] getCString:in_channels];
    char *interface;
    NSString *interfaccia = [NSString init];
    interfaccia = [interfaceInputBox titleOfSelectedItem];
    interface = (char*)malloc(sizeof(char)*[interfaccia length]+2);
    [interfaccia getCString:interface];
	
	OSStatus err = noErr;
	UInt32 size;
	AudioDeviceID vDevice = 0;
	Boolean isWritable;

	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
	if (err != noErr) 
		return false;

	int manyDevices = size / sizeof(AudioDeviceID);
	JPLog("number of audio devices: %ld\n", manyDevices);

	AudioDeviceID devices[manyDevices];
	err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, &devices);
	if (err != noErr)
 		return false;
 
	int i;
	for (i = 0; i < manyDevices; i++) {
		char name[256];
		CFStringRef nameRef;
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
		if (err != noErr) 
			return false;
			
		CFStringGetCString(nameRef, name, 256, kCFStringEncodingMacRoman);
		CFRelease(nameRef);
		
		if (strcmp(interface, name) == 0)   
			vDevice = devices[i];
	}	
    
    if (!availableSamplerate(vDevice, (Float64)atoi(samplerate))) {
        
        NSString *mess1 = NSLocalizedString(@"Warning:", nil);
        NSString *mess2 = NSLocalizedString(@"Device is already used with another sample rate, that will be changed...", nil);
        NSString *mess3 = NSLocalizedString(@"Yes", nil);
        NSString *mess4 = NSLocalizedString(@"No", nil);
        NSInteger res = NSRunCriticalAlertPanel(mess1, mess2, mess3, mess4, nil);
        
        if (res == 0)
            goto end;
    }

	NSString *deviceIDStr = [[NSNumber numberWithLong:vDevice] stringValue];
	[deviceIDStr getCString:interface];
		
	if (strcmp(out_channels,"0") == 0 && strcmp(in_channels,"0") == 0) {
		openJack("");
        goto end;
	}
	
    char stringa[512];
	memset(stringa, 0x0, 512);
	
	char drivername[128];
	getDeviceUIDFromID(vDevice,drivername);

    // Conditionnal start..
    SInt32 major;
    SInt32 minor;

    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    
    
    if (getVerboseLevel() != 0) {
        
        if (major == 10 && minor >= 5) {
                    
        #if defined(__i386__)
            //strcpy(stringa, "arch -i386 /usr/local/bin/./jackdmp -R -d ");
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -v -d ");
        #elif defined(__x86_64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v -d ");
        #elif defined(__ppc__)
            strcpy(stringa, "arch -ppc /usr/local/bin/./jackdmp -R  -v -d ");
        #elif defined(__ppc64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v -d ");
        #endif
            
        } else {
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v -d ");
        }
        
    } else {
        
        if (major == 10 && minor >= 5) {
        
        #if defined(__i386__)
            //strcpy(stringa, "arch -i386 /usr/local/bin/./jackdmp -R -d ");
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -d ");
        #elif defined(__x86_64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -d ");
        #elif defined(__ppc__)
            strcpy(stringa, "arch -ppc /usr/local/bin/./jackdmp -R -d ");
        #elif defined(__ppc64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -d ");
        #endif
        
        } else {
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -d ");
        }
    }
       
    strcat(stringa, driver);
	strcat(stringa, " -r ");
    strcat(stringa, samplerate);
    strcat(stringa, " -p ");
    strcat(stringa, buffersize);
    strcat(stringa, " -o ");
    strcat(stringa, out_channels);
	strcat(stringa, " -i ");
    strcat(stringa, in_channels);
	strcat(stringa, " -d ");
	strcat(stringa, "\"");
	strcat(stringa, drivername);
	strcat(stringa, "\"");
    
    id pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."),LOCSTR(@"Jack server is starting..."),nil,nil,nil);
    NSModalSession modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
    openJack(stringa);
    [NSApp endModalSession:modalSession];
    NSReleaseAlertPanel(pannelloDiAlert);
    
    StartNotification();
    
 	free(driver); 
	free(samplerate); 
	free(buffersize); 
	free(out_channels); 
	free(in_channels); 
	free(interface); 
    [POOL release];
    return true;
    
end:
  	free(driver); 
	free(samplerate); 
	free(buffersize); 
	free(out_channels); 
	free(in_channels); 
	free(interface); 
    [POOL release];
    return false;
}

- (IBAction) closeJackDeamon:(id) sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
	[managerWin orderOut:sender];
	
	StopNotification();
	
	[[JackConnections getSelf] stopTimer];
	
	if (checkJack() != 0) {    
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
	[outputChannels setEnabled:YES];
	[inputChannels setEnabled:YES];
	[driverBox setEnabled:YES];
	[interfaceInputBox setEnabled:YES];
    [interfaceOutputBox setEnabled:YES];
	[samplerateText setEnabled:YES];
    [hogBox setEnabled:YES];
	[routingBut setEnabled:NO];
    
    gJackRunning = false;
    [POOL release];
}

- (IBAction) closeJackDeamon1:(id) sender {
    id POOL = [[NSAutoreleasePool alloc] init];
	[managerWin orderOut:sender];
	
	StopNotification();
	
	[[JackConnections getSelf] stopTimer];
    
    closeJack2();
 	
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
	[outputChannels setEnabled:YES];
	[inputChannels setEnabled:YES];
	[driverBox setEnabled:YES];
	[interfaceInputBox setEnabled:YES];
    [interfaceOutputBox setEnabled:YES];
	[samplerateText setEnabled:YES];
    [hogBox setEnabled:YES];
	[routingBut setEnabled:NO];
    
    gJackRunning = false;
    [POOL release];
}

- (IBAction) reloadPref:(id) sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
    if ([interfaceInputBox titleOfSelectedItem]) 
		[[interfaceInputBox titleOfSelectedItem] getCString:selectedInputDevice];
        
    if ([interfaceOutputBox titleOfSelectedItem]) 
		[[interfaceOutputBox titleOfSelectedItem] getCString:selectedOutputDevice];

    [interfaceInputBox removeAllItems];
    [interfaceOutputBox removeAllItems];
    [outputChannels removeAllItems];
	[inputChannels removeAllItems];
    [samplerateText removeAllItems];
	
    [bufferText removeAllItems];
	[bufferText addItemWithTitle:@"32"];
    [bufferText addItemWithTitle:@"64"]; [bufferText addItemWithTitle:@"128"]; 
	[bufferText addItemWithTitle:@"256"]; [bufferText addItemWithTitle:@"512"]; 
	[bufferText addItemWithTitle:@"1024"]; [bufferText addItemWithTitle:@"2048"]; 
	[bufferText addItemWithTitle:@"4096"];
    
	[bufferText selectItemWithTitle:@"512"]; // select the 512 value
	
    // 19/10/09: non critial error...
	if (![self writeDevNames]) {
        [Utility error:'aud4']; 
     }
	
    // 19/10/09: non critial error...
    if (![self writeCAPref:sender]) {
        [Utility error:'aud4']; 
    }
    
    [POOL release];
}

/*
Scan current audio device properties : in/out channels, sampling rate, buffer size
*/

- (BOOL) writeCAPref:(id)sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
    OSStatus err;
    UInt32 size;
	int i, countSRin, countSRout;
    
    // Input channels
	UInt32 inChannels = 0;
	err = getTotalChannels(selInputDevID, &inChannels, true);
    if (err != noErr) { 
		NSLog(@"err in getTotalChannels");
		[inputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[inputChannels selectItemAtIndex:0];
	} else {
		[inputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[inputChannels selectItemAtIndex:0];
		for (i = 0; i < inChannels; i++) {
			[inputChannels addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[inputChannels selectItemAtIndex:i+1];
		}
		JPLog("got input channels ok, %d channels\n", inChannels);
    }
    
    // Output channels
    UInt32 outChannels = 0;
    err = getTotalChannels(selOutputDevID, &outChannels, false);
	if (err != noErr) { 
		NSLog(@"err in getTotalChannels, set to 0");
		[outputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[outputChannels selectItemAtIndex:0];
	} else {
		[outputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[outputChannels selectItemAtIndex:0];
		for (i = 0 ;i < outChannels; i++) {
			[outputChannels addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[outputChannels selectItemAtIndex:i+1];
		}
		JPLog("got output channels ok, %d channels\n",outChannels);
	}
    
    Float64 sampleRatesCombined[32];
	
    // Sampling rate for input
    countSRin = numberOfItemsInNominalSampleRateComboBox(selInputDevID);
    JPLog("numberOfItemsInNominalSampleRateComboBox for input: %ld\n",countSRin);
    for (i = 0; i < countSRin; i++) {
        Float64 rate = objectValueForItemAtIndex(selInputDevID, i);
        JPLog("Sample rate value for input = %ld \n", (long)rate);
        [samplerateText addItemWithTitle:[[NSNumber numberWithLong:(long)rate] stringValue]];
    }
    
    // Sampling rate for output
    countSRout = numberOfItemsInNominalSampleRateComboBox(selOutputDevID);
    JPLog("numberOfItemsInNominalSampleRateComboBox for output: %ld\n",countSRout);
    for (i = 0; i < countSRout; i++) {
        Float64 rate = objectValueForItemAtIndex(selOutputDevID, i);
        JPLog("Sample rate value for output = %ld \n", (long)rate);
        [samplerateText addItemWithTitle:[[NSNumber numberWithLong:(long)rate] stringValue]];
    }
	
 	// Buffer size
    UInt32 newBufFrame;
    size = sizeof(UInt32);
    err = AudioDeviceGetProperty(selDevID, 0, false, kAudioDevicePropertyBufferFrameSize, &size, &newBufFrame);
    if (err != noErr) 
        goto end;
    
	[bufferText selectItemWithTitle:@"512"]; // forcing to a lower value...
    JPLog("got actual buffersize ok %ld\n",newBufFrame);
    
    UInt32 theSize = sizeof(UInt32);
	UInt32 newBufSize = 32;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"32"]; 
    newBufSize = 64;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"64"]; 
    newBufSize = 128;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"128"]; 
    newBufSize = 256;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"256"]; 
    newBufSize = 512;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"512"]; 
    newBufSize = 1024;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"1024"];
    newBufSize = 2048;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"2048"];
    newBufSize = 4096;
    err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &newBufSize);
    if (err != noErr) 
		[bufferText removeItemWithTitle:@"4096"];

    JPLog("buffersize tests OK\n");
	
	UInt32 oldSize = newBufFrame;
	size = sizeof(UInt32);
    err = AudioDeviceGetProperty(selDevID, 0, false, kAudioDevicePropertyBufferFrameSize, &size, &newBufFrame);
    if (err != noErr) 
        goto end;
	
	JPLog("got actual buffersize OK %ld\n",newBufFrame);
    
	if (oldSize != newBufFrame) {
		JPLog("AudioDeviceSetProperty kAudioDevicePropertyBufferFrameSize %ld\n",oldSize);
		err = AudioDeviceSetProperty(selDevID, NULL, 0, false, kAudioDevicePropertyBufferFrameSize, theSize, &oldSize);
		if (err) { 
			NSLog(@"err in kAudioDevicePropertyBufferFrameSize"); 
            goto end; 
		}
	}
    
    JPLog("set old buffersize OK\n");
    [POOL release];
    return YES;
    
end:
    [POOL release];
    return NO;
}

/*
Scan audio devices and display their names in interfaceInputBox (Jack Router and iSight devices are filtered out)
Set the selDevID variable to the currently selected device of the system default device
*/

- (BOOL)writeDevNames {
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    AudioDeviceID defaultInputDev, defaultOuputDev;
    int i;
    int manyDevices;
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        [POOL release];
        return NO; 
    }
    
    manyDevices = size/sizeof(AudioDeviceID);
	JPLog("number of audio devices: %ld\n", manyDevices);
    
    AudioDeviceID devices[manyDevices];
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
    if (err != noErr) 
		goto end; 
        
    size = sizeof(AudioDeviceID);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &size, &defaultInputDev);
    if (err != noErr) 
		goto end;
        
    size = sizeof(AudioDeviceID);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &defaultOuputDev);
    if (err != noErr) 
		goto end; 
        
    BOOL selected = NO;
	JPLog("First selected input device: %s.\n", selectedInputDevice);
    
    for (i = 0; i < manyDevices; i++) {
		char name[256];
		CFStringRef nameRef;
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
        if (err != noErr) 
			goto end; 
		CFStringGetCString(nameRef, name, 256, kCFStringEncodingMacRoman);
		JPLog("Checking device: %s.\n", name);
		
     	if (strcmp(&name[0],"JackRouter") != 0 && strcmp(name,"iSight") != 0 && checkDevice(devices[i])) {
			JPLog("Adding device: %s.\n", name);
            NSString *s_name = [NSString stringWithCString:name encoding:NSMacOSRomanStringEncoding];
			[interfaceInputBox addItemWithTitle:s_name];
            [interfaceOutputBox addItemWithTitle:s_name];
            
            if (strcmp(selectedInputDevice, name) == 0) { 
				selected = YES; 
				[interfaceInputBox selectItemWithTitle:s_name]; 
				selInputDevID = devices[i]; 
				JPLog("Selected input device:: %s.\n", name);
			}
            
            // TODO 
            if (strcmp(selectedOutputDevice, name) == 0) { 
				selected = YES; 
				[interfaceOutputBox selectItemWithTitle:s_name]; 
				selOutputDevID = devices[i]; 
				JPLog("Selected output device:: %s.\n", name);
			}
        }
		
		CFRelease(nameRef);
    }
        
    if (!selected) {
		selInputDevID = defaultInputDev; 
        selOutputDevID = defaultOuputDev; 
    }
    
    [POOL release];
    return YES;
    
end:
    [POOL release];
    return NO;
}

#ifdef PLUGIN
- (void)testPlugin:(id)sender {
	JPPlugin *newPlugTest = [JPPlugin alloc];
	if ([newPlugTest open:@"JPPlugTest"]) {
		NSLog(@"Plugin opened, testing:");
		[newPlugTest openEditor];
		[newPlugTest jackStatusHasChanged:kJackIsOff];
		[newPlugTest jackStatusHasChanged:kJackIsOn];
		[newPlugTest release];
	}
}

- (void) openPlugin:(id)sender {
	JPPlugin *newPlug = [JPPlugin alloc];
	if ([newPlug open:[[sender menu] title]]) {
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
		for (i = 0; i < nItems; i++) {
			id it = [slotMenu itemAtIndex:i];
			if (![[it title] isEqualToString:[[sender menu] title]]) 
				[it setEnabled:NO];
		}
		
		id upMenu = [slotMenu supermenu];
		
		nItems = [upMenu numberOfItems];
		for (i = 0; i < nItems; i++) {
			id it = [upMenu itemAtIndex:i];
			if ([[it title] isEqualToString:[slotMenu title]])
				[it setState:NSOnState];
		}
		if (jackstat != 1) 
			[newPlug jackStatusHasChanged:kJackIsOff];
		else 
			[newPlug jackStatusHasChanged:kJackIsOn];
		
		[plugins_ids addObject:newPlug];
		[self addPluginSlot];
	} 
}

- (void) closePlugin:(id)sender {
	JPPlugin *plug = [[sender menu] delegate];
	
	[plugins_ids removeObject:plug];
	id item = [[sender menu] itemWithTitle:LOCSTR(@"Open Instance")];
	if (item) {
		[item setAction:@selector(openPlugin:)];
		[[[[sender menu] supermenu] itemWithTitle:[[sender menu] title]] setState:NSOffState];
	}
	id item2 = [[sender menu] itemWithTitle:LOCSTR(@"Close Instance")];
	[item2 setAction:nil];
	id item3 = [[sender menu] itemWithTitle:LOCSTR(@"Open Editor")];
	[item3 setAction:nil];
	
	id slotMenu = [[sender menu] supermenu];
	[slotMenu setAutoenablesItems:YES];

	int nItems = [slotMenu numberOfItems];
	int i;
	for (i = 0; i < nItems; i++) {
		id it = [slotMenu itemAtIndex:i];
		[it setEnabled:YES];
		id subItMenu = [it submenu];
		id itemToAble = [subItMenu itemWithTitle:LOCSTR(@"Open Instance")];
		[itemToAble setAction:@selector(openPlugin:)];
	}
	
	[slotMenu update];
	
	id upMenu = [slotMenu supermenu];
		
	nItems = [upMenu numberOfItems];
	for (i = 0; i < nItems; i++) {
		id it = [upMenu itemAtIndex:i];
		if ([[it title] isEqualToString:[slotMenu title]]) 
			[it setState:NSOffState];
	}
	
	[pluginsMenu removeItem:[pluginsMenu itemAtIndex:[pluginsMenu numberOfItems]-1]];
}

- (void) openPluginEditor:(id)sender {
	JPPlugin *plug = [[sender menu] delegate];
	[plug openEditor];
}
#endif

@end
