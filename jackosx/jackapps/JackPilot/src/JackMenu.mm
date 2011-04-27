/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "JackMenu.h"
#import "JPPlugin.h"
#import "JackCon1.3.h"

#include <set>

#include <sys/sysctl.h>
#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreAudio/CoreAudio.h>

using namespace std;

typedef	UInt8 CAAudioHardwareDeviceSectionID;

#define	kAudioDeviceSectionGlobal	((CAAudioHardwareDeviceSectionID)0x00)

static bool gJackRunning = false;

static OSStatus getTotalChannels(AudioDeviceID device, UInt32* channelCount, Boolean isInput);

static void JackInfoShutDown(jack_status_t code, const char* reason, void *arg) 
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
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates \n");
        }
        
        count = size / sizeof(AudioValueRange);
        JPLog("Sample rate different values: %ld\n",count);
	
        AudioValueRange valueTable[count];
        err = AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, valueTable);
        if (err != noErr) {
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates\n");
        }
        
        AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &valueTable);
        if (err != noErr) {
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates\n");
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
		//theAnswer = 1; ??
        theAnswer = 0;
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
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates\n");
        }
        
        count = size / sizeof(AudioValueRange);
        JPLog("Sample rate different values: %ld\n",count);
	
        AudioValueRange valueTable[count];
        err = AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, valueTable);
        if (err != noErr) {
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates\n");
        }
        
        AudioDeviceGetProperty(device, 0, true, kAudioDevicePropertyAvailableNominalSampleRates, &size, &valueTable);
        if (err != noErr) {
            JPLog("err in (info) kAudioDevicePropertyAvailableNominalSampleRates\n");
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

static bool checkDevice(AudioDeviceID device)
{
    OSStatus err;
    
    JPLog("checkDevice\n");
	
	// Input channels
	UInt32 inChannels = 0;
	err = getTotalChannels(device, &inChannels, true);
    if (err != noErr) { 
		JPLog("checkDevice : err in getTotalChannels\n");
		return false;
	} 
    
    // Output channels
    UInt32 outChannels = 0;
    err = getTotalChannels(device, &outChannels, false);
    if (err != noErr) { 
		JPLog("checkDevice : err in getTotalChannels\n");
		return false;
	} 
	
	JPLog("checkDevice input/output  device = %ld input = %ld output = %ld\n", device, inChannels, outChannels);
	return (outChannels == 0) && (inChannels == 0) ? false : true;
}

static bool checkDeviceName(const char* deviceName)
{
    OSStatus err;
    UInt32 size;
    Boolean isWritable;
    int i;
    
    JPLog("checkDeviceName\n");
    
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

static OSStatus getDeviceUIDFromID(AudioDeviceID id, char* name)
{
    JPLog("getDeviceUIDFromID\n");
     
    UInt32 size = sizeof(CFStringRef);
	CFStringRef UI;
    OSStatus res = AudioDeviceGetProperty(id, 0, false, kAudioDevicePropertyDeviceUID, &size, &UI);
	if (res == noErr) {
		CFStringGetCString(UI, name, 128, CFStringGetSystemEncoding());
		JPLog("getDeviceUIDFromID: name = %s\n", name);
		CFRelease(UI);
	} else {	
        name[0] = 0;
		JPLog("getDeviceUIDFromID: error name = %s\n", name);
	}
    return res;
}

static bool isSameClockDomain(AudioDeviceID input, AudioDeviceID output)
{
    UInt32 input_clockdomain = 0;
    UInt32 output_clockdomain = 0;
    UInt32  outSize = sizeof(UInt32);
    OSStatus osErr;
    
    osErr = AudioDeviceGetProperty(input, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &input_clockdomain); 
    if (osErr != noErr) {
        JPLog("Cannot get clock domain for input");
        return false;
    }
    osErr = AudioDeviceGetProperty(output, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyClockDomain, &outSize, &output_clockdomain); 
    if (osErr != noErr) {
        JPLog("Cannot get clock domain for output");
        return false;
    }
    
    if (input_clockdomain == output_clockdomain && input_clockdomain != 0) {
        return true;
    } else {
        return false;
    }
}

static bool isAggregateDevice(AudioDeviceID device)
{
    JPLog("isAggregateDevice\n");
     
    UInt32 deviceType, outSize = sizeof(UInt32);
    OSStatus err = AudioDeviceGetProperty(device, 0, kAudioDeviceSectionGlobal, kAudioDevicePropertyTransportType, &outSize, &deviceType);
    
    if (err != noErr) {
        JPLog("kAudioDevicePropertyTransportType error");
        return false;
    } else {
        return (deviceType == kAudioDeviceTransportTypeAggregate);
    }
}

static OSStatus getTotalChannels(AudioDeviceID device, UInt32* channelCount, Boolean isInput) 
{
    OSStatus			err = noErr;
    UInt32				outSize;
    Boolean				outWritable;
    AudioBufferList*	bufferList = 0;
	unsigned int i;
    
    JPLog("getTotalChannels\n");
	
	*channelCount = 0;
    err = AudioDeviceGetPropertyInfo(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
    if (err == noErr) {
        bufferList = (AudioBufferList*)malloc(outSize);
        err = AudioDeviceGetProperty(device, 0, isInput, kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
        if (err == noErr) {								
            for (i = 0; i < bufferList->mNumberBuffers; i++) 
                *channelCount += bufferList->mBuffers[i].mNumberChannels;
        } else {
            JPLog("getTotalChannels : AudioDeviceGetProperty error\n");
        }
		if (bufferList) 
			free(bufferList);	
    } else {
        JPLog("getTotalChannels : AudioDeviceGetPropertyInfo error\n");
    }
	return (err);
}

static bool isDuplexDevice(AudioDeviceID device)
{
    JPLog("isDuplexDevice\n");
 
    UInt32 inChannels = 0;
	OSStatus err1 = getTotalChannels(device, &inChannels, true);
    
    UInt32 outChannels = 0;
	OSStatus err2 = getTotalChannels(device, &outChannels, false);
    
    return (err1 == noErr && err2 == noErr && inChannels > 0 && outChannels > 0);
}

static bool availableOneSamplerate(AudioDeviceID device, Float64 wantedSampleRate)
{
    OSStatus err = noErr;
    UInt32 outSize;
    UInt32 used; 
    Float64 usedSampleRate;
    
    JPLog("availableOneSamplerate\n");
     
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
    
    JPLog("availableSamplerate\n");
    
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

    JPLog("awakeFromNib\n");
    
    id POOL = [[NSAutoreleasePool alloc] init];
	
	plugins_ids = [[NSMutableArray array] retain];
    
    BOOL needsPref = NO;
    if ([Utility initPreference]) 
		needsPref = YES;

    [self writeHomePath];
   
     if (jackALLoad() == 1) {
     
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
            
        if (getClockMode() > 0)
			[clockBox setState:NSOnState];
		else 
			[clockBox setState:NSOffState];
            
        if (getMonitorMode() > 0)
			[monitorBox setState:NSOnState];
		else 
			[monitorBox setState:NSOffState];
            
        if (getMIDIMode() > 0)
			[MIDIBox setState:NSOnState];
		else 
			[MIDIBox setState:NSOffState];    
    } 
    
    int test = checkJack();
    if (test != 0) { 
		gJackRunning = openJackClient();
        if (gJackRunning)
            [self setPrefItem:NO];
      
        //[[JackConnections getSelf] JackCallBacks]; // not used
        jack_on_info_shutdown(getClient(), JackInfoShutDown, self);
        jack_activate(getClient());
        
		jackstat = 1; 
		writeStatus(1);
		[isonBut setStringValue:LOCSTR(@"Jack is On")];
		[self setupTimer]; 
		[startBut setTitle:LOCSTR(@"Stop")];
		[toggleDock setTitle:LOCSTR(@"Stop")];
		[bufferText setEnabled:NO];
		[outputChannels setEnabled:NO];
		[inputChannels setEnabled:NO];
		[driverBox setEnabled:NO];
		[interfaceInputBox setEnabled:NO];
        [interfaceOutputBox setEnabled:NO];
		[samplerateText setEnabled:NO];
        [hogBox setEnabled:NO];
        [clockBox setEnabled:NO];
        [monitorBox setEnabled:NO];
        [MIDIBox setEnabled:NO];
       
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
            
			char deviceInputName[256];
            char deviceOutputName[256];
            
			[[prefs objectAtIndex:1] getCString:deviceInputName];
         	[[prefs objectAtIndex:2] getCString:deviceOutputName];
            
			JPLog("Reading preferences ref file deviceInputName = %s deviceOutputName = %s\n", deviceInputName, deviceOutputName);
			if ((checkDeviceName(deviceInputName) || checkDeviceName(deviceOutputName)) && ([prefs count] == 7)) { // Check if devices kept in preference are available
				[driverBox selectItemWithTitle:[prefs objectAtIndex:0]];
                if (checkDeviceName(deviceInputName))
                    [interfaceInputBox selectItemWithTitle:[prefs objectAtIndex:1]];
                if (checkDeviceName(deviceOutputName))
                    [interfaceOutputBox selectItemWithTitle:[prefs objectAtIndex:2]];
         		[samplerateText selectItemWithTitle:[prefs objectAtIndex:3]];
				[bufferText selectItemWithTitle:[prefs objectAtIndex:4]];
				[outputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
				[inputChannels selectItemWithTitle:[prefs objectAtIndex:6]];
			}  else if (!getHogMode()) {  // Otherwise opens the preference dialog
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
	
// Plugins-Menu stuff:
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
    if (prefs && ([prefs count] == 7)) {
    
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
	
	if (![Utility savePref:toPrefs prefType:'winP'])
        JPLog("Cannot save windows positions\n");
	
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
		JPLog("Cannot plugins instances\n");
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

-(void)setPrefItem:(BOOL) state
{
    NSMenu *mainMenu = [NSApp mainMenu];
    NSMenuItem *jpItem = [mainMenu itemAtIndex:0];
    NSMenu *menu = [jpItem submenu];
    [menu setAutoenablesItems:NO];
    NSMenuItem *prefItem = [menu itemAtIndex:2];
    [prefItem setEnabled:state];
}

- (IBAction)startJack:(id)sender
{	
    id POOL = [[NSAutoreleasePool alloc] init];
    
    JPLog("startJack\n");
    
	if (![self launchJackDeamon:sender])
        return;
    
	char driverInputname[128];
    char driverOutputname[128];
    
	getDeviceUIDFromID(selOutputDevID, driverOutputname);
    getDeviceUIDFromID(selInputDevID, driverInputname);
    
    if ([JALauto state] == NSOnState) {
        jackALStore([JALin intValue],[JALout intValue],1,
					[defInput state] == NSOnState ? TRUE : FALSE,
					[defOutput state] == NSOnState ? TRUE : FALSE,
					[sysDefOut state] == NSOnState ? TRUE : FALSE,
					[verboseBox state] == NSOnState ? 1 : 0,
					driverInputname,
                    driverOutputname,
                    [hogBox state] == NSOnState ? 1 : 0,
                    [clockBox state] == NSOnState ? 1 : 0,
                    [monitorBox state] == NSOnState ? 1 : 0,
                    [MIDIBox state] == NSOnState ? 1 : 0);
    }
	
    if ([JALauto state] == NSOffState) {
		jackALStore([JALin intValue],[JALout intValue],0,
					[defInput state] == NSOnState ? TRUE : FALSE,
					[defOutput state] == NSOnState ? TRUE : FALSE,
					[sysDefOut state] == NSOnState ? TRUE : FALSE,
					[verboseBox state] == NSOnState ? 1 : 0,
					driverInputname,
                    driverOutputname,
                    [hogBox state] == NSOnState ? 1 : 0,
                    [clockBox state] == NSOnState ? 1 : 0,
                    [monitorBox state] == NSOnState ? 1 : 0,
                    [MIDIBox state] == NSOnState ? 1 : 0);
    }	
	
    gJackRunning = openJackClient();
    if (gJackRunning)
        [self setPrefItem:NO];
    
    if (checkJack() != 0 && getClient() != NULL){
        jackstat = 1;
        writeStatus(1); 
        
        //[[JackConnections getSelf] JackCallBacks]; // not used
        jack_on_info_shutdown(getClient(), JackInfoShutDown, self);
        jack_activate(getClient());
        
        [isonBut setStringValue:LOCSTR(@"Jack is On")];
        [startBut setTitle:LOCSTR(@"Stop")]; 
        [toggleDock setTitle:LOCSTR(@"Stop")];
        [self setupTimer];
        [bufferText setEnabled:NO];
        [outputChannels setEnabled:NO];
        [inputChannels setEnabled:NO];
        [driverBox setEnabled:NO];
        [interfaceInputBox setEnabled:NO];
        [interfaceOutputBox setEnabled:NO];
        [samplerateText setEnabled:NO];
        [hogBox setEnabled:NO];
        [clockBox setEnabled:NO];
        [monitorBox setEnabled:NO];
        [MIDIBox setEnabled:NO];
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
    int nClients = quantiClienti() - 1;
    
    /* 29/10/09 : removed
	if ([[outputChannels titleOfSelectedItem] intValue] == 0 && [[inputChannels titleOfSelectedItem] intValue] == 0) 
		nClients++;
    */
        
    int a;
    
	if (nClients > 1 || (nClients == 1 && !getMIDIMode())) {
     	NSString *youhave = LOCSTR(@"You have ");
		NSString *mess = LOCSTR(@" clients running, they will stop working or maybe crash!!");
        if (getMIDIMode()) nClients--;
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
    JPLog("jackALstore\n");
    
    // Do not save when running...
    if (gJackRunning)
        return;

    id POOL = [[NSAutoreleasePool alloc] init];
    
	char driverInputname[128];
    char driverOutputname[128];
    
	getDeviceUIDFromID(selOutputDevID, driverOutputname);
    getDeviceUIDFromID(selInputDevID, driverInputname);
 	
    if ([JALauto state] == NSOnState) {
        if (jackALStore([JALin intValue],[JALout intValue],1,
						[defInput state] == NSOnState ? TRUE : FALSE,
						[defOutput state] == NSOnState ? TRUE : FALSE,
						[sysDefOut state] == NSOnState ? TRUE : FALSE,
						[verboseBox state] == NSOnState ? 1 : 0,
						driverInputname,
                        driverOutputname,
                        [hogBox state] == NSOnState ? 1 : 0,
                        [clockBox state] == NSOnState ? 1 : 0,
                        [monitorBox state] == NSOnState ? 1 : 0,
                        [MIDIBox state] == NSOnState ? 1 : 0) == 0) 
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
                        [hogBox state] == NSOnState ? 1 : 0,
                        [clockBox state] == NSOnState ? 1 : 0,
                        [monitorBox state] == NSOnState ? 1 : 0,
                        [MIDIBox state] == NSOnState ? 1 : 0) == 0)  
            [self error:@"Cannot save JAS preferences."];
    }
	
	// Save the preferences in a format jack clients can use when using the jack_client_open API
	char filename[255];
	snprintf(filename, 255, "%s/.jackdrc", getenv("HOME"));
	FILE* file = fopen(filename,"w");
    
    if (file) {
    
        fprintf(file,"/usr/local/bin/jackd \n");
        if (getMIDIMode()) {
            fprintf(file, "-X coremidi \n"); 
        }
        fprintf(file,"-R \n");
        fprintf(file, "-d %s \n",[[driverBox titleOfSelectedItem]cString]);
        fprintf(file, "-p %s \n",[[bufferText titleOfSelectedItem]cString]);
        fprintf(file, "-r %s \n",[[samplerateText titleOfSelectedItem]cString]);
        fprintf(file, "-i %s \n",[[inputChannels titleOfSelectedItem]cString]);
        fprintf(file, "-o %s \n",[[outputChannels titleOfSelectedItem]cString]);
        
        if (strcmp(driverInputname, driverOutputname) == 0) {
            fprintf(file, "-d %s \n", driverInputname); 
        } else {
            fprintf(file, "-C %s \n", driverInputname); 
            fprintf(file, "-P %s \n", driverOutputname); 
        }
        
        if (getHogMode()) {
            fprintf(file, "-H \n"); 
        }
        
        if (getClockMode()) {
            fprintf(file, "-s \n"); 
        }
        
        if (getMonitorMode()) {
            fprintf(file, "-m \n"); 
        }
         
        fclose(file);
	}
   
    NSMutableArray *toFile;
    
    toFile = [NSMutableArray array];
    [toFile addObject:[driverBox titleOfSelectedItem]];
    
    if ([interfaceInputBox titleOfSelectedItem] != nil) {
        [toFile addObject:[interfaceInputBox titleOfSelectedItem]];
    } else {
        [toFile addObject:@"Unknown"];
    }
    if ([interfaceOutputBox titleOfSelectedItem] != nil) {
        [toFile addObject:[interfaceOutputBox titleOfSelectedItem]];
    } else {
        [toFile addObject:@"Unknown"];
    }
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
    commando = (char*)calloc(256, sizeof(char*));
    res = (char*)calloc(256, sizeof(char*));
    buf2 = (char*)calloc(256, sizeof(char*));
    NSBundle *pacco = [NSBundle bundleForClass:[self class]];
    NSString *path = [pacco resourcePath];
    NSArray *lista = [path componentsSeparatedByString:@" "];
    UInt32 i;
    for (i = 0; i < [lista count]; i++) {
        id item = [lista objectAtIndex:i];
        [item getCString:buf2];
        strcat(res, buf2);
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
		JPLog("JAS not found\n");
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
        JPLog("JackFramework not found\n");
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
        JPLog("Bundle not found\n");
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
    char *command = (char*)calloc(256, sizeof(char));
    strcpy(command,"open http://www.jackosx.com");
	my_system2(command);
    free(command);
}

-(IBAction)openJohnnyMail:(id)sender {
    char *command = (char*)calloc(256, sizeof(char));
    strcpy(command,"open mailto:johnny@meskalina.it");
    my_system2(command);
    free(command);
}

-(IBAction)openAMS:(id)sender {
    char *command = (char*)calloc(256, sizeof(char));
    strcpy(command,"open \"/Applications/Utilities/Audio\ MIDI\ Setup.app\"");
    my_system2(command);
    free(command);
}

-(IBAction)openSP:(id)sender {
    char *command = (char*)calloc(256, sizeof(char));
    strcpy(command,"open \"/System/Library/PreferencePanes/Sound.prefPane\"");
    my_system2(command);
    free(command);
}

- (IBAction)openPrefWin:(id)sender {

    JPLog("openPrefWin\n");
    
    [self reloadPref:sender];
    
    if (sender) {
        NSArray *prefs = [Utility getPref:'audi'];
        if (prefs && ([prefs count] == 7)) {
            BOOL needsReload = NO;
            
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
            if (![[samplerateText title] isEqualToString:[prefs objectAtIndex:3]]) 
                [samplerateText selectItemAtIndex:0];
            
            [bufferText selectItemWithTitle:[prefs objectAtIndex:4]];
            if (![[bufferText title] isEqualToString:[prefs objectAtIndex:4]]) 
                [bufferText selectItemAtIndex:0];
            
            [outputChannels selectItemWithTitle:[prefs objectAtIndex:5]];
            if (![[outputChannels title] isEqualToString:[prefs objectAtIndex:5]]) { 
                needsReload = YES; 
                [outputChannels selectItemAtIndex:0]; 
            }
            
            [inputChannels selectItemWithTitle:[prefs objectAtIndex:6]];
            if (![[inputChannels title] isEqualToString:[prefs objectAtIndex:6]]) { 
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
    modalSex = [NSApp beginModalSessionForWindow:aboutWin];
}

-(IBAction) closeAboutWin:(id)sender {
    [NSApp endModalSession:modalSex];
    [aboutWin orderOut:sender];
}

- (void)closeFromCallback {
    [self closeJackDeamon:0];
}

- (bool) launchJackDeamon:(id) sender {
    
    NSModalSession modalSession;
    id pannelloDiAlert;
    
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
	
    if (!availableSamplerate(selInputDevID, (Float64)atoi(samplerate)) || !availableSamplerate(selOutputDevID, (Float64)atoi(samplerate))) {
        
        NSString *mess1 = NSLocalizedString(@"Warning:", nil);
        NSString *mess2 = NSLocalizedString(@"Device is already used with another sample rate, that will be changed...", nil);
        NSString *mess3 = NSLocalizedString(@"Yes", nil);
        NSString *mess4 = NSLocalizedString(@"No", nil);
        int res = NSRunCriticalAlertPanel(mess1, mess2, mess3, mess4, nil);
        
        if (res == 0)
            goto end;
    }
    
    // Nothing to open..
	if (strcmp(out_channels,"0") == 0 && strcmp(in_channels,"0") == 0) {
        goto end;
	}
	
    char stringa[512];
	memset(stringa, 0, 512);
   
    char driverInputname[128];
    char driverOutputname[128];
    
	getDeviceUIDFromID(selInputDevID, driverInputname);
 	getDeviceUIDFromID(selOutputDevID, driverOutputname);

    // Conditionnal start..
    SInt32 major;
    SInt32 minor;

    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    
    if (getVerboseLevel() != 0) {
        
        if (major == 10 && minor >= 5) {
                    
        #if defined(__i386__)
            //strcpy(stringa, "arch -i386 /usr/local/bin/./jackdmp -R -d ");
            strcpy(stringa,"/usr/local/bin/./jackdmp -R -v");
        #elif defined(__x86_64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v");
        #elif defined(__ppc__)
            strcpy(stringa, "arch -ppc /usr/local/bin/./jackdmp -R  -v");
        #elif defined(__ppc64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v");
        #endif
            
        } else {
            strcpy(stringa,"/usr/local/bin/./jackdmp -R  -v");
        }
        
        if (getMIDIMode()) {
            strcat(stringa, " -X coremidi ");
        }
        
        strcat(stringa, " -d ");
        
    } else {
        
        if (major == 10 && minor >= 5) {
        
        #if defined(__i386__)
            //strcpy(stringa, "arch -i386 /usr/local/bin/./jackdmp -R -d ");
            strcpy(stringa,"/usr/local/bin/./jackdmp -R");
        #elif defined(__x86_64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R");
        #elif defined(__ppc__)
            strcpy(stringa, "arch -ppc /usr/local/bin/./jackdmp -R");
        #elif defined(__ppc64__)
            strcpy(stringa,"/usr/local/bin/./jackdmp -R");
        #endif
        
        } else {
            strcpy(stringa,"/usr/local/bin/./jackdmp -R");
        }
        
        if (getMIDIMode()) {
            strcat(stringa, " -X coremidi ");
        }
        
        strcat(stringa, " -d ");

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
    
    if (strcmp(driverInputname, driverOutputname) == 0) {
        strcat(stringa, " -d ");
        strcat(stringa, "\"");
        strcat(stringa, driverInputname);
        strcat(stringa, "\"");
    } else {
        strcat(stringa, " -C ");
        strcat(stringa, "\"");
        strcat(stringa, driverInputname);
        strcat(stringa, "\"");  
        strcat(stringa, " -P ");
        strcat(stringa, "\"");
        strcat(stringa, driverOutputname);
        strcat(stringa, "\"");  
    }
    
    if (getHogMode()) {
        strcat(stringa, " -H ");
    }
    
    if (getClockMode()) {
        strcat(stringa, " -s ");
    }
    
    if (getMonitorMode()) {
        strcat(stringa, " -m ");
    }
        
    pannelloDiAlert = NSGetAlertPanel(LOCSTR(@"Please Wait..."), LOCSTR(@"Jack server is starting..."), nil, nil, nil);
    modalSession = [NSApp beginModalSessionForWindow:pannelloDiAlert];
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
	[startBut setTitle:LOCSTR(@"Start")];
	[toggleDock setTitle:LOCSTR(@"Start")];
	[connectionsNumb setIntValue:0];
	[bufferText setEnabled:YES];
	[outputChannels setEnabled:YES];
	[inputChannels setEnabled:YES];
	[driverBox setEnabled:YES];
	[interfaceInputBox setEnabled:YES];
    [interfaceOutputBox setEnabled:YES];
	[samplerateText setEnabled:YES];
    [hogBox setEnabled:YES];
    [clockBox setEnabled:YES];
    [monitorBox setEnabled:YES];
    [MIDIBox setEnabled:YES];
	[routingBut setEnabled:NO];
    
    gJackRunning = false;
    [self setPrefItem:YES];
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
	[startBut setTitle:LOCSTR(@"Start")];
	[toggleDock setTitle:LOCSTR(@"Start")];
	[connectionsNumb setIntValue:0];
	[bufferText setEnabled:YES];
	[outputChannels setEnabled:YES];
	[inputChannels setEnabled:YES];
	[driverBox setEnabled:YES];
	[interfaceInputBox setEnabled:YES];
    [interfaceOutputBox setEnabled:YES];
	[samplerateText setEnabled:YES];
    [hogBox setEnabled:YES];
    [clockBox setEnabled:YES];
    [monitorBox setEnabled:YES];
    [MIDIBox setEnabled:YES];
	[routingBut setEnabled:NO];
    
    gJackRunning = false;
    [self setPrefItem:YES];
    [POOL release];
}

- (IBAction) reloadPref:(id) sender {

    JPLog("reloadPref\n");
    
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
    
    JPLog("reloadPref selectedInputDevice : %s\n", selectedInputDevice);
    JPLog("reloadPref selectedOutputDevice : %s\n", selectedOutputDevice);
 	
    // When device is "hogged" it's properties cannot be accessed anymore...
    if (!(gJackRunning && getHogMode())) {
    
        // 19/10/09: non critial error...
        if (![self writeDevNames]) {
            [Utility error:'aud4']; 
         }
        
        // 19/10/09: non critial error...
        if (![self writeCAPref:sender]) {
            [Utility error:'aud4']; 
        }
    }
    
    [POOL release];
}

/*
Scan current audio device properties : in/out channels, sampling rate, buffer size
*/

static bool checkBufferSizeRange(AudioValueRange& input,  AudioValueRange& output, Float64 buffer_size)
{
    Float64 frame_size = buffer_size;
    return (frame_size >= input.mMinimum && frame_size <= input.mMaximum 
            && frame_size >= output.mMinimum && frame_size <= output.mMaximum);
}

- (BOOL) writeCAPref:(id)sender {
    
    id POOL = [[NSAutoreleasePool alloc] init];
    
    OSStatus err;
    UInt32 size;
	UInt32 countSRin = 0;
    UInt32 countSRout = 0;
    UInt32 i;
    
    JPLog("writeCAPref\n");
    
    // Input channels
 	UInt32 inChannels = 0;
	err = getTotalChannels(selInputDevID, &inChannels, true);
    if (err != noErr) { 
		JPLog("writeCAPref: err in getTotalChannels\n");
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
		JPLog("writeCAPref: err in getTotalChannels\n");
		[outputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[outputChannels selectItemAtIndex:0];
	} else {
		[outputChannels addItemWithTitle:[[NSNumber numberWithInt:0] stringValue]]; 
		[outputChannels selectItemAtIndex:0];
		for (i = 0; i < outChannels; i++) {
			[outputChannels addItemWithTitle:[[NSNumber numberWithInt:i+1] stringValue]];
			[outputChannels selectItemAtIndex:i+1];
		}
		JPLog("got output channels ok, %d channels\n", outChannels);
	}
   
    set<Float64> commonset;
    set<Float64> inputset;
    set<Float64> outputset;
    
    // Sampling rate for input
    countSRin = numberOfItemsInNominalSampleRateComboBox(selInputDevID);
    JPLog("numberOfItemsInNominalSampleRateComboBox for input: %ld\n",countSRin);
    for (i = 0; i < countSRin; i++) {
        Float64 rate = objectValueForItemAtIndex(selInputDevID, i);
        JPLog("Sample rate value for input = %ld \n", (int)rate);
        inputset.insert(rate);
        commonset.insert(rate);
    }
    
    // Sampling rate for output
    countSRout = numberOfItemsInNominalSampleRateComboBox(selOutputDevID);
    JPLog("numberOfItemsInNominalSampleRateComboBox for output: %ld\n",countSRout);
    for (i = 0; i < countSRout; i++) {
        Float64 rate = objectValueForItemAtIndex(selOutputDevID, i);
        JPLog("Sample rate value for output = %ld \n", (int)rate);
        outputset.insert(rate);
        commonset.insert(rate);
    }
    
    set<Float64>::const_iterator it;
    if (countSRin > 0 && countSRout > 0) {
        // Only display common values
        JPLog("input and output SR size all = %ld\n", commonset.size());
        for (it = commonset.begin(); it != commonset.end(); it++) {
            if (inputset.find(*it) != inputset.end() && outputset.find(*it) != outputset.end()) 
                [samplerateText addItemWithTitle:[[NSNumber numberWithLong:(int)(*it)] stringValue]];
        }
    } else if (countSRin > 0) {
        // Input only
        JPLog("input SR size = %ld\n", inputset.size());
        for (it = inputset.begin(); it != inputset.end(); it++) {
            [samplerateText addItemWithTitle:[[NSNumber numberWithLong:(int)(*it)] stringValue]];
        }
        
    } else if (countSRout > 0) {
        // Output only
        JPLog("output SR size = %ld\n", outputset.size());
        for (it = outputset.begin(); it != outputset.end(); it++) {
            [samplerateText addItemWithTitle:[[NSNumber numberWithLong:(int)(*it)] stringValue]];
        }
    }
 	
    // Get buffer size range for input and output
    size = sizeof(AudioValueRange);
    
    AudioValueRange inputRange;
    err = AudioDeviceGetProperty(selInputDevID, 0, true, kAudioDevicePropertyBufferFrameSizeRange, &size, &inputRange);
    if (err != noErr) {
        JPLog("Cannot get buffer size range for input\n");
        // Set default...
        inputRange.mMinimum = 32;
        inputRange.mMaximum = 4096;
    } else {
        JPLog("Get buffer size range for input min = %d max = %d\n", (int)(inputRange.mMinimum), (int)(inputRange.mMaximum));
    }
      
    AudioValueRange outputRange;
    err = AudioDeviceGetProperty(selOutputDevID, 0, false, kAudioDevicePropertyBufferFrameSizeRange, &size, &outputRange);
    if (err != noErr) {
        JPLog("Cannot get buffer size range for output\n");
        // Set default...
        outputRange.mMinimum = 32;
        outputRange.mMaximum = 4096;
    } else {
        JPLog("Get buffer size range for output min = %d max = %d\n", (int)(outputRange.mMinimum), (int)(outputRange.mMaximum));
    }
         
    if (checkBufferSizeRange(inputRange, outputRange, 32))
        [bufferText addItemWithTitle:@"32"];
    
    if (checkBufferSizeRange(inputRange, outputRange, 64))
        [bufferText addItemWithTitle:@"64"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 128))
        [bufferText addItemWithTitle:@"128"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 256))
        [bufferText addItemWithTitle:@"256"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 512))
        [bufferText addItemWithTitle:@"512"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 1024))
        [bufferText addItemWithTitle:@"1024"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 2048))
        [bufferText addItemWithTitle:@"2048"];
        
    if (checkBufferSizeRange(inputRange, outputRange, 4096))
        [bufferText addItemWithTitle:@"4096"];
        
    [bufferText selectItemWithTitle:@"512"];
    
    [POOL release];
    return YES;
}

/*
Scan audio devices and display their names in interfaceInputBox and interfaceOutputBox (JackRouter and iSight devices are filtered out)
Set the selDevID variable to the currently selected device of the system default device
*/

- (BOOL)writeDevNames {
    OSStatus err;
    OSStatus err1, err2;
    UInt32 size;
    Boolean isWritable;
    AudioDeviceID defaultInputDev = 0;
    AudioDeviceID defaultOuputDev = 0;
    int i;
    int manyDevices;
    BOOL selectedIn = NO;
    BOOL selectedOut = NO;
    
    bool new_selected_in = false;
    bool new_selected_out = false;
    
    int first_input_index = -1;
    int first_output_index = -1;
        
    NSString *s_name_in = NULL; 
    NSString *s_name_out = NULL; 
    
    JPLog("writeDevNames\n");
     
    id POOL = [[NSAutoreleasePool alloc] init];
    
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, &isWritable);
    if (err != noErr) {
        [POOL release];
        return NO; 
    }
    
    manyDevices = size/sizeof(AudioDeviceID);
	JPLog("Number of audio devices = %ld\n", manyDevices);
    
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
   
    for (i = 0; i < manyDevices; i++) {
		char name[256];
		CFStringRef nameRef;
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
        if (err != noErr) 
			goto end; 
		CFStringGetCString(nameRef, name, 256, kCFStringEncodingMacRoman);
		JPLog("Checking device = %s\n", name);
		
     	if (strcmp(name,"JackRouter") != 0 && strcmp(name,"iSight") != 0 && checkDevice(devices[i])) {
            JPLog("Adding device = %s\n", name);
            NSString *s_name = [NSString stringWithCString:name encoding:NSMacOSRomanStringEncoding];
            
            UInt32 inChannels = 0;
            err1 = getTotalChannels(devices[i], &inChannels, true);
            
            UInt32 outChannels = 0;
            err2 = getTotalChannels(devices[i], &outChannels, false);
            
            // Filters AD that are not duplex...
            if (err1 == noErr && err2 == noErr && isAggregateDevice(devices[i]) && (inChannels == 0 || outChannels == 0)) {
                continue;
            }
            
            if (err1 == noErr && inChannels > 0) {
                if (first_input_index < 0) 
                    first_input_index = i;
                [interfaceInputBox addItemWithTitle:s_name];
            }
            
            if (err2 == noErr && outChannels > 0) {
                if (first_output_index < 0) 
                    first_output_index = i;
                [interfaceOutputBox addItemWithTitle:s_name];
            }
            
            if ((strcmp(selectedInputDevice, name) == 0) && (inChannels > 0)) { 
				selectedIn = YES; 
				[interfaceInputBox selectItemWithTitle:s_name]; 
                s_name_in = [NSString stringWithCString:name encoding:NSMacOSRomanStringEncoding];
                new_selected_in = (selInputDevID != devices[i]);
				selInputDevID = devices[i]; 
       			JPLog("Selected input device = %s\n", name);
			}
            
            if ((strcmp(selectedOutputDevice, name) == 0) && (outChannels > 0)) { 
				selectedOut = YES; 
				[interfaceOutputBox selectItemWithTitle:s_name]; 
                s_name_out = [NSString stringWithCString:name encoding:NSMacOSRomanStringEncoding];
                new_selected_out = (selOutputDevID != devices[i]);
				selOutputDevID = devices[i]; 
                JPLog("Selected output device = %s\n", name);
			}
        }
		
		CFRelease(nameRef);
    }
   
    // Setup devices depending if they are duplex or not....
    if (new_selected_in) {
         if (isDuplexDevice(selInputDevID)) {
            [interfaceInputBox selectItemWithTitle:s_name_in]; 
            [interfaceOutputBox selectItemWithTitle:s_name_in]; 
            selOutputDevID = selInputDevID;
         } else if (isDuplexDevice(selOutputDevID)) {
            [interfaceInputBox selectItemWithTitle:s_name_in]; 
            [interfaceOutputBox selectItemAtIndex:0];
            selOutputDevID = devices[first_output_index];
         } else {
            [interfaceInputBox selectItemWithTitle:s_name_in];
            [interfaceOutputBox selectItemWithTitle:s_name_out]; 
         }
    }
    
    if (new_selected_out) {
        if (isDuplexDevice(selOutputDevID)) {
            [interfaceInputBox selectItemWithTitle:s_name_out]; 
            [interfaceOutputBox selectItemWithTitle:s_name_out]; 
            selInputDevID = selOutputDevID;
        } else if (isDuplexDevice(selInputDevID)) {
            [interfaceInputBox selectItemAtIndex:0]; 
            [interfaceOutputBox selectItemWithTitle:s_name_out]; 
            selInputDevID = devices[first_input_index];
        } else {
            [interfaceInputBox selectItemWithTitle:s_name_in];
            [interfaceOutputBox selectItemWithTitle:s_name_out]; 
        }
    } 
    
    // Clock drift compensation checkbox state...
    if (isDuplexDevice(selOutputDevID) || isDuplexDevice(selInputDevID)) {
        [clockBox setState:NSOffState];
        [clockBox setEnabled:NO];
    } else {
        [clockBox setEnabled:YES];
    }
          
    if (!selectedIn) {
		selInputDevID = defaultInputDev; 
    }
    
    if (!selectedOut) {
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
		JPLog("Plugin opened, testing: \n");
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
