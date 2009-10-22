//
//  JCPreferencePane.m
//  JackControl
//
//  Created by Ev on 1/26/06.
//  Copyright (c) 2006 __MyCompanyName__. All rights reserved.
//

#import "JCPreferencePane.h"
#import <CoreAudio/CoreAudio.h>
#import <sys/sysctl.h>
#import <unistd.h>

static int jackConnectionChangedCallback(void *arg)
{
	[(JCPreferencePane *)arg performSelectorOnMainThread:@selector(updateConnectionCountDisplay) withObject:nil waitUntilDone:NO];
	return 0;
}

static void jackShutdownCallback(void *arg)
{
	[(JCPreferencePane *)arg performSelectorOnMainThread:@selector(jackWillShutdown) withObject:nil waitUntilDone:NO];
}

@implementation JCPreferencePane

- (void)mainViewDidLoad
{
    NSBundle *bundle;
	
	jackID = nil;
	
    bundle = [NSBundle bundleWithIdentifier:@"com.grame.Jack"];
	if (bundle != nil)
	{
		[jackVersionText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleVersion"]];
		[jackInfoText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"]];
	}
	else
	{
		[jackVersionText setStringValue:JCLocalizedString(@"Version Not Installed")];
		[jackInfoText setStringValue:JCLocalizedString(@"Info Not Installed")];
	}
    
	bundle = [NSBundle bundleWithIdentifier:@"com.grame.JackAudioServer"];
	if (!bundle) bundle = [NSBundle bundleWithIdentifier:@"com.grame.JackRouter"];
	if (bundle != nil)
	{
		[jackRouterVersionText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleVersion"]];
		[jackRouterInfoText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"]];
	}
	else
	{
		[jackRouterVersionText setStringValue:JCLocalizedString(@"Version Not Installed")];
		[jackRouterInfoText setStringValue:JCLocalizedString(@"Info Not Installed")];
    }
	
    bundle = [NSBundle bundleForClass:[self class]];
	[jackControlVersionText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleVersion"]];
	[jackControlInfoText setStringValue:[bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"]];
}

- (void)willSelect
{
	NSDictionary *defaults = [[NSUserDefaults standardUserDefaults] persistentDomainForName:[[NSBundle bundleForClass:[self class]] bundleIdentifier]];
	
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(jackWillStart:) name:JackWillStartNotification object:nil];

	cpuUsageDisplayTimer = [[NSTimer timerWithTimeInterval:1.0 target:self selector:@selector(updateCPUUsageDisplay:) userInfo:nil repeats:YES] retain];
    [[NSRunLoop currentRunLoop] addTimer:cpuUsageDisplayTimer forMode:NSDefaultRunLoopMode];

	if (jackState = [self isJackRunning])
	{
		jackID = jack_client_new("JackControl");
		jack_set_graph_order_callback(jackID, jackConnectionChangedCallback, self);
		jack_on_shutdown(jackID, jackShutdownCallback, self);
		jack_activate(jackID);
	}
	
	// setup hardware controls
	{
		id obj;
		
		[self buildAudioDevicePopup];
		
		obj = [defaults objectForKey:@"Interface"];
		if (obj) [interfacePopup selectItemWithTitle:obj];
		
		[self buildAudioDriverPopup];
		
		obj = [defaults objectForKey:@"Driver"];
		[driverPopup selectItemWithTitle:(obj ? obj : @"coreaudio")];
		
		[self buildSampleRatesPopup];
		
		obj = [defaults objectForKey:@"SampleRate"];
		[sampleRatePopup selectItemWithTag:(obj ? [obj intValue] : 44100)];
		
		[self buildBufferSizesPopup];
				
		obj = [defaults objectForKey:@"BufferSize"];
		[bufferSizePopup selectItemWithTag:(obj ? [obj intValue] : 512)];
		
		[self buildChannelCountSteppers];
		
		obj = [defaults objectForKey:@"InterfaceInputChannels"];
		[interfaceInputChannelsStepper setIntValue:(obj ? [obj intValue] : 2)];
		[interfaceInputChannelsText setIntValue:[interfaceInputChannelsStepper intValue]];

		obj = [defaults objectForKey:@"InterfaceOutputChannels"];
		[interfaceOutputChannelsStepper setIntValue:(obj ? [obj intValue] : 2)];
		[interfaceOutputChannelsText setIntValue:[interfaceOutputChannelsStepper intValue]];
	}
	
	// setup server controls
	{
		id obj;
		
		obj = [defaults objectForKey:@"ApplicationInputChannels"];
		[applicationInputChannelsStepper setIntValue:(obj ? [obj intValue] : 2)];
		[applicationInputChannelsText setIntValue:[applicationInputChannelsStepper intValue]];

		obj = [defaults objectForKey:@"ApplicationOutputChannels"];
		[applicationOutputChannelsStepper setIntValue:(obj ? [obj intValue] : 2)];
		[applicationOutputChannelsText setIntValue:[applicationOutputChannelsStepper intValue]];
		
		obj = [defaults objectForKey:@"AutomaticallyConnect"];
		[automaticallyConnectButton setState:(obj ? [obj boolValue] : YES)];
		
		obj = [defaults objectForKey:@"ShowStatusMenu"];
		[menuStatusButton setState:(obj ? [obj boolValue] : YES)];
		
		obj = [defaults objectForKey:@"StartAtLogin"];
		[startAtLoginButton setState:(obj ? [obj boolValue] : NO)];
		
		[self writeServerSettingsFile];
	}
	
	[self update];
}

- (void)willUnselect
{
    NSDictionary *defaults;
	
	// write defaults to com.jackosx.JackControl.plist
	defaults = [NSDictionary dictionaryWithObjectsAndKeys:
					[interfacePopup titleOfSelectedItem], @"Interface",
					[driverPopup titleOfSelectedItem], @"Driver",
					[NSNumber numberWithInt:[sampleRatePopup selectedTag]], @"SampleRate",
					[NSNumber numberWithInt:[bufferSizePopup selectedTag]], @"BufferSize",
					[NSNumber numberWithInt:[interfaceInputChannelsText intValue]], @"InterfaceInputChannels",
					[NSNumber numberWithInt:[interfaceOutputChannelsText intValue]], @"InterfaceOutputChannels",
					[NSNumber numberWithInt:[applicationInputChannelsText intValue]], @"ApplicationInputChannels",
					[NSNumber numberWithInt:[applicationOutputChannelsText intValue]], @"ApplicationOutputChannels",
					[NSNumber numberWithBool:[automaticallyConnectButton state]], @"AutomaticallyConnect",
					[NSNumber numberWithBool:[menuStatusButton state]], @"ShowStatusMenu",
					[NSNumber numberWithBool:[startAtLoginButton state]], @"StartAtLogin",
					nil];
	[[NSUserDefaults standardUserDefaults] setPersistentDomain:defaults forName:[[NSBundle bundleForClass:[self class]] bundleIdentifier]];

	// also write to ~/.jackdrc
	NSString *jackdrc = [NSString stringWithFormat:@"/usr/local/bin/jackd \n-R \n-d %@ \n-r %d \n-p %d \n-o %d \n-i %d \n-n %@ \n",
							[driverPopup titleOfSelectedItem],
							[sampleRatePopup selectedTag],
							[bufferSizePopup selectedTag],
							[interfaceOutputChannelsText intValue],
							[interfaceInputChannelsText intValue],
							[[interfacePopup selectedItem] representedObject],
							nil];
	NSString *jackdrcPath = @"~/.jackdrc";
	
	[jackdrc writeToFile:[jackdrcPath stringByExpandingTildeInPath] atomically:NO];
	
	[cpuUsageDisplayTimer invalidate];
    [cpuUsageDisplayTimer release];
    cpuUsageDisplayTimer = nil;

	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:JackWillStartNotification object:nil];

	if (jackID != nil)
	{
		jack_client_close(jackID);
		jackID = nil;
	}
	
}

//
#pragma mark UPDATE
//

- (void)update
{	
	switch (jackState)
	{
		case kJackIsNotRunning:
			[runningTitleText setStringValue:JCLocalizedString(@"Stopped Title")];
			[runningDescriptionText setStringValue:JCLocalizedString(@"Stopped Description")];
			[startButton setTitle:JCLocalizedString(@"Stopped Button")];
			break;
		case kJackIsRunning:
			[runningTitleText setStringValue:JCLocalizedString(@"Running Title")];
			[runningDescriptionText setStringValue:JCLocalizedString(@"Running Description")];
			[startButton setTitle:JCLocalizedString(@"Running Button")];
			break;
		case kJackIsLaunching:
			[runningTitleText setStringValue:JCLocalizedString(@"Starting Title")];
			[runningDescriptionText setStringValue:JCLocalizedString(@"General Description")];
			[startButton setTitle:JCLocalizedString(@"Stopped Button")];
			break;
		case kJackIsTerminating:
			[runningTitleText setStringValue:JCLocalizedString(@"Stopping Title")];
			[runningDescriptionText setStringValue:JCLocalizedString(@"General Description")];
			[startButton setTitle:JCLocalizedString(@"Running Button")];
			break;
	}
	
	[startButton setEnabled:(jackState == kJackIsNotRunning || jackState == kJackIsRunning)];
	
	[driverPopup setEnabled:(jackState == kJackIsNotRunning)];
	[interfacePopup setEnabled:(jackState == kJackIsNotRunning)];
	[sampleRatePopup setEnabled:(jackState == kJackIsNotRunning)];
	[bufferSizePopup setEnabled:(jackState == kJackIsNotRunning)];
	[interfaceInputChannelsText setEnabled:(jackState == kJackIsNotRunning)];
	[interfaceInputChannelsStepper setEnabled:(jackState == kJackIsNotRunning)];
	[interfaceOutputChannelsText setEnabled:(jackState == kJackIsNotRunning)];
	[interfaceOutputChannelsStepper setEnabled:(jackState == kJackIsNotRunning)];
		
	[uninstallButton setEnabled:(jackState == kJackIsNotRunning)];

	[self updateCPUUsageDisplay:nil];
	[self updateConnectionCountDisplay];
}

- (void)updateCPUUsageDisplay:(id)userInfo
{
	NSString *s;
	
	if (jackID != nil)
	{
		float load = jack_cpu_load(jackID);
		if (load < 0.5)
			s = @"<1%";
		else
			s = [NSString stringWithFormat:@"%.0f%%", load];
	}
	else
		s = @"--";
	
	[cpuUsageText setStringValue:s];
}

- (void)updateConnectionCountDisplay
{
	if (jackID != nil)
		[connectionsText setIntValue:[self numberOfJackConnections]];
	else
		[connectionsText setStringValue:@"--"];
}

//
#pragma mark ACTIONS
//

- (IBAction)startStop:(id)sender
{
	if (![self isJackRunning])
	{
		// if is multiprocessor and if found, start jackdmp instead?
		NSString *command = [NSString stringWithFormat:@"/usr/local/bin/./jackd -R -d %@ -r %d -p %d -o %d -i %d - \"%@\"",
								[driverPopup titleOfSelectedItem],
								[sampleRatePopup selectedTag],
								[bufferSizePopup selectedTag],
								[interfaceOutputChannelsText intValue],
								[interfaceInputChannelsText intValue],
								[[interfacePopup selectedItem] representedObject],
								nil];
		
		if (fork() == 0)
		{
			execl("/bin/sh", "/bin/sh", "-c", [command UTF8String], nil);
			_exit(EXIT_FAILURE);
		}
		
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:JackWillStartNotification object:nil];
	}
	else
	{		
		if ([self numberOfJackConnections] == 0)
		{
			if (jackID != nil)
			{
				jack_client_close(jackID);
				jackID = nil;
			}
			
			int processID = [self jackProcessID];
			if (processID != 0)
				kill(processID, SIGQUIT);
			
			jackState = kJackIsTerminating;
			[self update];

			launchTimer = [[NSTimer timerWithTimeInterval:2.0 target:self selector:@selector(finishTerminatingJack:) userInfo:nil repeats:NO] retain];
			[[NSRunLoop currentRunLoop] addTimer:launchTimer forMode:NSDefaultRunLoopMode];
		}
		else
		{
			NSBeginAlertSheet(JCLocalizedString(@"Stop Error Title"), JCLocalizedString(@"OK"), @"", @"", [[self mainView] window], nil, nil, nil, nil, JCLocalizedString(@"Stop Error Description"));
		}
	}
}

- (void)jackWillStart:(id)userInfo
{
	jackState = kJackIsLaunching;
	[self update];

	launchTimer = [[NSTimer timerWithTimeInterval:2.0 target:self selector:@selector(finishLaunchingJack:) userInfo:nil repeats:NO] retain];
	[[NSRunLoop currentRunLoop] addTimer:launchTimer forMode:NSDefaultRunLoopMode]; 
}

- (void)jackWillShutdown
{
	jack_deactivate(jackID);
	jack_client_close(jackID);
	jackID = nil;

	launchTimer = [[NSTimer timerWithTimeInterval:2.0 target:self selector:@selector(finishTerminatingJack:) userInfo:nil repeats:NO] retain];
	[[NSRunLoop currentRunLoop] addTimer:launchTimer forMode:NSDefaultRunLoopMode]; 
}

- (void)finishLaunchingJack:(id)userInfo
{
	jackID = jack_client_new("JackControl");
	
	if (jackID == nil)
	{
		int processID = [self jackProcessID];
		if (processID != 0)
			kill(processID, SIGQUIT);

		jackState = kJackIsNotRunning;
		NSBeginAlertSheet(JCLocalizedString(@"Start Error Title"), JCLocalizedString(@"OK"), @"", @"", [[self mainView] window], nil, nil, nil, nil, JCLocalizedString(@"Start Error Description"));
	}
	else
	{
		jack_set_graph_order_callback(jackID, jackConnectionChangedCallback, self);
		jack_on_shutdown(jackID, jackShutdownCallback, self);
		jack_activate(jackID);
		jackState = kJackIsRunning;
	}
		
	[self update];

    [launchTimer release];
    launchTimer = nil;
}

- (void)finishTerminatingJack:(id)userInfo
{
	jackState = kJackIsNotRunning;
	[self update];

    [launchTimer release];
    launchTimer = nil;
}

- (IBAction)interfaceChanged:(id)sender
{
	[self buildSampleRatesPopup];
	[self buildBufferSizesPopup];
	[self buildChannelCountSteppers];
	[self writeServerSettingsFile];
}

- (IBAction)serverSettingChanged:(id)sender
{
	if (sender == applicationInputChannelsStepper)
		[applicationInputChannelsText setIntValue:[sender intValue]];
	else if (sender == applicationInputChannelsText)
		[applicationInputChannelsStepper setIntValue:[sender intValue]];
	else if (sender == applicationOutputChannelsStepper)
		[applicationOutputChannelsText setIntValue:[sender intValue]];
	else if (sender == applicationOutputChannelsText)
		[applicationOutputChannelsStepper setIntValue:[sender intValue]];
	
	[self writeServerSettingsFile];
}

- (IBAction)menuStatusChanged:(id)sender
{
	NSDictionary *info = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:[menuStatusButton state]] forKey:@"state"];
	[[NSDistributedNotificationCenter defaultCenter] postNotificationName:JackStatusMenuStateNotification object:nil userInfo:info];
}

- (IBAction)launchJackPanel:(id)sender
{
    [[NSWorkspace sharedWorkspace] launchApplication:@"JackPanel"];
}

- (IBAction)launchAudioMIDISetup:(id)sender
{
    [[NSWorkspace sharedWorkspace] launchApplication:@"Audio MIDI Setup"];
}

- (IBAction)uninstall:(id)sender
{
	if (NSRunAlertPanel(JCLocalizedString(@"Uninstall Title"), JCLocalizedString(@"Uninstall Message"), JCLocalizedString(@"No"), JCLocalizedString(@"Yes"), @"") == NSAlertAlternateReturn)
		[[NSWorkspace sharedWorkspace] openFile:[[NSBundle bundleForClass:[self class]] pathForResource:@"uninstall" ofType:@"command"]];
}

- (IBAction)homePage:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.jackosx.com/"]];
}

//
#pragma mark BUILDING
//

- (void)buildAudioDevicePopup
{
	UInt32 size;
	int numOfDevices;
	AudioDeviceID *deviceList;
	NSString *deviceName, *deviceUID;
	OSStatus err;
	int i;
	
	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, nil);
	numOfDevices = size / sizeof(AudioDeviceID);
	deviceList = (AudioDeviceID*)malloc(size); 
	err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, deviceList);
	
	[interfacePopup removeAllItems];
	for (i=0;i<numOfDevices;i++)
	{
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(deviceList[i], 0, NO, kAudioDevicePropertyDeviceNameCFString, &size, &deviceName);
		size = sizeof(CFStringRef);
		err = AudioDeviceGetProperty(deviceList[i], 0, NO, kAudioDevicePropertyDeviceUID, &size, &deviceUID);
		if (![deviceName isEqualToString:@"Jack Router"] && ![deviceName isEqualToString:@"iSight"])
		{
			[interfacePopup addItemWithTitle:deviceName];
			[[interfacePopup itemWithTitle:deviceName] setTag:deviceList[i]];
			[[interfacePopup itemWithTitle:deviceName] setRepresentedObject:deviceUID];
		}
		[deviceName release];
		[deviceUID release];
	}
		
	free(deviceList);
}

- (void)buildAudioDriverPopup
{
	//NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/usr/local/lib/jack/"];
    NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager] enumeratorAtPath:@"/usr/local/lib/jackmp/"];
	NSString *file;
	
	[driverPopup removeAllItems];
	while (file = [enumerator nextObject])
	{
		if ([file hasPrefix:@"jack_"])
			[driverPopup addItemWithTitle:[[file stringByDeletingPathExtension] substringFromIndex:5]];
	}
}

- (void)buildSampleRatesPopup
{
	AudioDeviceID deviceID = [interfacePopup selectedTag];
	NSString *title;
	UInt32 size;
	Float64 rate;
	int numOfItems;
	OSStatus err;
	int i;
	
	if (deviceID == -1) return;
	
	err = AudioDeviceGetPropertyInfo(deviceID, 0, NO, kAudioDevicePropertyAvailableNominalSampleRates, &size, nil);
	numOfItems = size / sizeof(AudioValueRange);
	AudioValueRange values[numOfItems];
	err = AudioDeviceGetProperty(deviceID, 0, NO, kAudioDevicePropertyAvailableNominalSampleRates, &size, values);
	
	[sampleRatePopup removeAllItems];
	for (i=0;i<numOfItems;i++)
	{
		title = [NSString stringWithFormat:@"%.0f Hz", values[i].mMinimum];
		[sampleRatePopup addItemWithTitle:title];
		[[sampleRatePopup itemWithTitle:title] setTag:(int)values[i].mMinimum];
	}
	
	size = sizeof(rate);
	err = AudioDeviceGetProperty(deviceID, 0, NO, kAudioDevicePropertyNominalSampleRate, &size, &rate);
	[sampleRatePopup selectItemWithTag:(int)rate];
}

- (void)buildBufferSizesPopup
{
	AudioDeviceID deviceID = [interfacePopup selectedTag];
	NSString *title;
	UInt32 currentBufferSize, bufferSize;
	UInt32 size = sizeof(bufferSize);
	OSStatus err;
	int i;
	
	if (deviceID == -1) return;

    AudioDeviceGetProperty(deviceID, 0, NO, kAudioDevicePropertyBufferFrameSize, &size, &currentBufferSize);

	[bufferSizePopup removeAllItems];
	for(i=5;i<13;i++)
	{
		bufferSize = pow(2, i);
		err = AudioDeviceSetProperty(deviceID, nil, 0, NO, kAudioDevicePropertyBufferFrameSize, size, &bufferSize);
		if (err == noErr)
		{
			title = [NSString stringWithFormat:@"%d", bufferSize];
			[bufferSizePopup addItemWithTitle:title];
			[[bufferSizePopup itemWithTitle:title] setTag:bufferSize];
		}
	}

	AudioDeviceSetProperty(deviceID, nil, 0, NO, kAudioDevicePropertyBufferFrameSize, size, &currentBufferSize);
	[bufferSizePopup selectItemWithTag:currentBufferSize];
}

- (void)buildChannelCountSteppers
{
	AudioDeviceID deviceID = [interfacePopup selectedTag];
	int channelCount;
	UInt32 size;
	AudioBufferList *bufferList;
	int i;

	if (deviceID == -1) return;

    AudioDeviceGetPropertyInfo(deviceID, 0, YES, kAudioDevicePropertyStreamConfiguration, &size, nil);
	bufferList = (AudioBufferList*)malloc(size);
	AudioDeviceGetProperty(deviceID, 0, YES, kAudioDevicePropertyStreamConfiguration, &size, bufferList);
    channelCount = 0;
	for (i=0;i<bufferList->mNumberBuffers;i++) 
		channelCount += bufferList->mBuffers[i].mNumberChannels;
	free(bufferList);

	[interfaceInputChannelsStepper setMaxValue:channelCount];
	[interfaceInputChannelsText setIntValue:channelCount];

    AudioDeviceGetPropertyInfo(deviceID, 0, NO, kAudioDevicePropertyStreamConfiguration, &size, nil);
	bufferList = (AudioBufferList*)malloc(size);
	AudioDeviceGetProperty(deviceID, 0, NO, kAudioDevicePropertyStreamConfiguration, &size, bufferList);
    channelCount = 0;
	for (i=0;i<bufferList->mNumberBuffers;i++) 
		channelCount += bufferList->mBuffers[i].mNumberChannels;
	free(bufferList);

	[interfaceOutputChannelsStepper setMaxValue:channelCount];
	[interfaceOutputChannelsText setIntValue:channelCount];
}

//
#pragma mark JACK ACCESS
//

- (BOOL)isJackRunning
{
	return ([self jackProcessID] != 0);
}

- (int)jackProcessID
{
	static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t length;
	int processID = 0;

	if (sysctl((int *)name, 4, nil, &length, nil, 0) == 0)
	{
		struct kinfo_proc *result = malloc(length);
		if (sysctl((int *)name, 4, result, &length, nil, 0) == 0)
		{
			int i, count = length / sizeof(struct kinfo_proc);
		
			for(i=0;i<count && !processID;i++)
				if(!strcmp("jackd", result[i].kp_proc.p_comm))
				{
					if (result[i].kp_proc.p_stat != SZOMB)
						processID = result[i].kp_proc.p_pid;
				}
		}
		free(result);
	}

	return processID;
}

- (int)numberOfJackConnections
{
	const char **ports;
	char *portName;
	jack_port_t *port;
	const char **connections;
	unsigned int p, c;
	unsigned int count = 0;
	
	if (jackID != nil)
	{
		ports = jack_get_ports(jackID, nil, nil, 0);
		if (ports != nil)
		{
			p = 0;
			while (portName = (char *)ports[p++])
			{
				if (port = jack_port_by_name(jackID, portName))
				{
					if (jack_port_flags(port) & JackPortIsOutput)
					{
						connections = jack_port_get_all_connections(jackID, port);
						if (connections != nil)
						{
							c = 0;
							while (connections[c++])
								count++;
						}
						free(connections);
					}
				}
			}
			free(ports);
		}
	}
	
	return count;
}

- (void)writeServerSettingsFile
{
	NSString *jas = [NSString stringWithFormat:@"\t%d\t-1\t%d\t-1\t%d\t-1\t0\t-1\t0\t-1\t0\t-1\t0\t-1\t%@",
							[applicationInputChannelsText intValue],
							[applicationOutputChannelsText intValue],
							[automaticallyConnectButton state],
							[[interfacePopup selectedItem] representedObject],
							nil];
	NSString *jasPath = @"~/Library/Preferences/JAS.jpil";
	
	[jas writeToFile:[jasPath stringByExpandingTildeInPath] atomically:NO];
}

@end
