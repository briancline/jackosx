#import <Cocoa/Cocoa.h>
#import <jack/jack.h>

enum
{
	kJackIsNotRunning = 0,
	kJackIsRunning,
	kJackIsLaunching,
	kJackIsTerminating
};

#define JackWillStartNotification @"JackWillStart"
#define JackStatusMenuStateNotification @"JackStatusMenuState"

@interface JAAgent : NSObject
{
	unsigned int jackState;
	jack_client_t* jackID;

	NSStatusItem *item;
	IBOutlet NSMenu *menu;

	NSTimer *launchTimer;
}

- (void)update;
- (void)jackWillStart:(id)userInfo;
- (void)jackWillShutdown;
- (void)finishLaunchingJack:(id)userInfo;
- (void)finishTerminatingJack:(id)userInfo;

- (IBAction)jackState:(id)sender;
- (IBAction)toggleJack:(id)sender;
- (IBAction)connections:(id)sender;
- (IBAction)cpuLoad:(id)sender;
- (IBAction)openJackPreferences:(id)sender;
- (IBAction)openJackPanel:(id)sender;

- (BOOL)isJackRunning;
- (int)jackProcessID;
- (NSString *)jackConnectionsString;
- (int)numberOfJackConnections;
- (NSString *)jackCPULoad;

@end
