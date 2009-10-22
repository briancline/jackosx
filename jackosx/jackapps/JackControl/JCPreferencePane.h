//
//  JCPreferencePane.h
//  JackControl
//
//  Created by Evan Olcott on 1/26/06.
//  Copyright (c) 2006 __MyCompanyName__. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>
#import <jack/jack.h>

#define JCLocalizedString(s) NSLocalizedStringFromTableInBundle(s, nil, [NSBundle bundleForClass:[self class]], @"")

enum
{
	kJackIsNotRunning = 0,
	kJackIsRunning,
	kJackIsLaunching,
	kJackIsTerminating
};

#define JackWillStartNotification @"JackWillStart"
#define JackStatusMenuStateNotification @"JackStatusMenuState"

@interface JCPreferencePane : NSPreferencePane 
{
	unsigned int jackState;
	jack_client_t* jackID;

	IBOutlet NSButton *startButton;
	IBOutlet NSTextField *runningTitleText;
	IBOutlet NSTextField *runningDescriptionText;
	
	IBOutlet NSTextField *connectionsText;
	IBOutlet NSTextField *cpuUsageText;
	
	IBOutlet NSPopUpButton *driverPopup;
	IBOutlet NSPopUpButton *interfacePopup;
	IBOutlet NSPopUpButton *sampleRatePopup;
	IBOutlet NSPopUpButton *bufferSizePopup;
	IBOutlet NSTextField *interfaceInputChannelsText;
	IBOutlet NSStepper *interfaceInputChannelsStepper;
	IBOutlet NSTextField *interfaceOutputChannelsText;
	IBOutlet NSStepper *interfaceOutputChannelsStepper;
	
	IBOutlet NSTextField *applicationInputChannelsText;
	IBOutlet NSStepper *applicationInputChannelsStepper;
	IBOutlet NSTextField *applicationOutputChannelsText;
	IBOutlet NSStepper *applicationOutputChannelsStepper;
	IBOutlet NSButton *automaticallyConnectButton;
	IBOutlet NSButton *menuStatusButton;
	IBOutlet NSButton *startAtLoginButton;
	
	IBOutlet NSTextField *jackVersionText;
	IBOutlet NSTextField *jackInfoText;
	IBOutlet NSTextField *jackRouterVersionText;
	IBOutlet NSTextField *jackRouterInfoText;
	IBOutlet NSTextField *jackControlVersionText;
	IBOutlet NSTextField *jackControlInfoText;
	
	IBOutlet NSButton *uninstallButton;
	
	NSTimer *cpuUsageDisplayTimer;
	NSTimer *launchTimer;
}

- (void)update;
- (void)updateCPUUsageDisplay:(id)userInfo;
- (void)updateConnectionCountDisplay;

- (IBAction)startStop:(id)sender;
- (void)jackWillStart:(id)userInfo;
- (void)jackWillShutdown;
- (void)finishLaunchingJack:(id)userInfo;
- (void)finishTerminatingJack:(id)userInfo;

- (IBAction)interfaceChanged:(id)sender;
- (IBAction)serverSettingChanged:(id)sender;
- (IBAction)menuStatusChanged:(id)sender;
- (IBAction)launchJackPanel:(id)sender;
- (IBAction)launchAudioMIDISetup:(id)sender;

- (void)buildAudioDevicePopup;
- (void)buildAudioDriverPopup;
- (void)buildSampleRatesPopup;
- (void)buildBufferSizesPopup;
- (void)buildChannelCountSteppers;

- (BOOL)isJackRunning;
- (int)jackProcessID;
- (int)numberOfJackConnections;
- (void)writeServerSettingsFile;

@end
