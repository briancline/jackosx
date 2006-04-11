#import "JAAgent.h"
#import <sys/sysctl.h>
#import <unistd.h>

extern jack_client_t *jack_client_new(const char *client_name) __attribute__((weak_import));

static void jackShutdownCallback(void *arg)
{
	[(JAAgent *)arg performSelectorOnMainThread:@selector(jackWillShutdown) withObject:nil waitUntilDone:NO];
}

@implementation JAAgent

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{	
	NSString *prefPath;
	NSDictionary *prefs;
	BOOL showMenu = YES;
	BOOL start = NO;

	jackID = nil;
	item = nil;
	
	prefPath = @"~/Library/Preferences/com.jackosx.JackControl.plist";
	
	prefs = [NSDictionary dictionaryWithContentsOfFile:[prefPath stringByExpandingTildeInPath]];
	if (prefs != nil)
	{
		id obj;
		
		obj = [prefs objectForKey:@"ShowStatusMenu"];
		if (obj) showMenu = [obj boolValue];
		
		obj = [prefs objectForKey:@"StartAtLogin"];
		if (obj) start = [obj boolValue];
	}
		
	// register for notifications (from self & from JackControl)
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(jackWillStart:) name:JackWillStartNotification object:nil];
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(menuStateChanged:) name:JackStatusMenuStateNotification object:nil];
	
	// leave if jack library not installed
	if (jack_client_new == nil)
		[NSApp terminate:self];

	// start up jack (be silent if error)
	if (start)
	{
		NSString *jackdrcPath = @"~/.jackdrc";
	
		NSMutableString *command = [NSMutableString stringWithContentsOfFile:[jackdrcPath stringByExpandingTildeInPath]];
		[command replaceOccurrencesOfString:@"\n" withString:@"" options:0 range:NSMakeRange(0, [command length])];
		
		if (fork() == 0)
		{
			execl("/bin/sh", "/bin/sh", "-c", [command UTF8String], nil);
			_exit(EXIT_FAILURE);
		}
		
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:JackWillStartNotification object:nil];
	}
	
	// add the status item
	if (showMenu)
	{
		item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
		[item setHighlightMode:YES];
		[item setMenu:menu];
	}

	[self update];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	// remove status item
	[[NSStatusBar systemStatusBar] removeStatusItem:item];
	[item release];
	
	// remove observer
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:JackWillStartNotification object:nil];
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:JackStatusMenuStateNotification object:nil];

	if (jackID != nil)
		jack_client_close(jackID);
}

//
#pragma mark UPDATE
//

- (void)update
{	
	NSString *imageName;
	NSString *hilitedImageName;
	
	switch (jackState)
	{
		case kJackIsNotRunning:
			imageName = @"disabled";
			hilitedImageName = @"disabled_highlighted";
			break;
		case kJackIsRunning:
			imageName = @"enabled";
			hilitedImageName = @"enabled_highlighted";
			break;
		case kJackIsLaunching:
			imageName = @"enabled_dimmed";
			hilitedImageName = @"enabled_highlighted";
			break;
		case kJackIsTerminating:
			imageName = @"disabled";
			hilitedImageName = @"disabled_highlighted";
			break;
	}

    [item setImage:[NSImage imageNamed:imageName]];
    [item setAlternateImage:[NSImage imageNamed:hilitedImageName]];
}

- (void)menuStateChanged:(NSNotification *)aNotification
{
	BOOL state = [[[aNotification userInfo] objectForKey:@"state"] boolValue];
	
	if (state)
	{
		item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
		[item setHighlightMode:YES];
		[item setMenu:menu];
		[self update];
	}
	else
	{
		[[NSStatusBar systemStatusBar] removeStatusItem:item];
		[item release];
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
	jackID = jack_client_new("JackAgent");
	
	if (jackID == nil)
	{
		int processID = [self jackProcessID];
		if (processID != 0)
			kill(processID, SIGQUIT);

		jackState = kJackIsNotRunning;
	}
	else
	{
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

//
#pragma mark MENU
//

- (IBAction)jackState:(id)sender
{
	// do nothing
}

- (IBAction)toggleJack:(id)sender
{
	if (![self isJackRunning])
	{
		NSString *jackdrcPath = @"~/.jackdrc";
	
		NSMutableString *command = [NSMutableString stringWithContentsOfFile:[jackdrcPath stringByExpandingTildeInPath]];
		[command replaceOccurrencesOfString:@"\n" withString:@"" options:0 range:NSMakeRange(0, [command length])];
		
		if (fork() == 0)
		{
			execl("/bin/sh", "/bin/sh", "-c", [command UTF8String], nil);
			_exit(EXIT_FAILURE);
		}
		
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:JackWillStartNotification object:nil];
	}
	else
	{
		jack_deactivate(jackID);
		jack_client_close(jackID);
		jackID = nil;

		int processID = [self jackProcessID];
		if (processID != 0)
			kill(processID, SIGQUIT);

		jackState = kJackIsTerminating;
		[self update];

		launchTimer = [[NSTimer timerWithTimeInterval:2.0 target:self selector:@selector(finishTerminatingJack:) userInfo:nil repeats:NO] retain];
		[[NSRunLoop currentRunLoop] addTimer:launchTimer forMode:NSDefaultRunLoopMode];
	}

}

- (IBAction)connections:(id)sender
{
	// do nothing
}

- (IBAction)cpuLoad:(id)sender
{
	// do nothing
}

- (IBAction)openJackPreferences:(id)sender
{
	NSString *path;
	
	path = @"/Library/PreferencePanes/Jack.prefPane";
	if (![[NSFileManager defaultManager] fileExistsAtPath:path])
		path = @"~/Library/PreferencePanes/Jack.prefPane";
	
	[[NSWorkspace sharedWorkspace] openFile:[path stringByExpandingTildeInPath]]; 
}

- (IBAction)openJackPanel:(id)sender
{
    [[NSWorkspace sharedWorkspace] launchApplication:@"JackPanel"];
}

- (BOOL)validateMenuItem:(id <NSMenuItem>)menuItem
{	
	SEL action = [menuItem action];
	BOOL isRunning = [self isJackRunning];
	BOOL enabled = YES;
		
	if (action == @selector(jackState:))
	{
		[menuItem setTitle:(isRunning ? NSLocalizedString(@"Jack On", @"") : NSLocalizedString(@"Jack Off", @""))];
		enabled = NO;
	}
	else if (action == @selector(toggleJack:))
	{
		[menuItem setTitle:(isRunning ? NSLocalizedString(@"Disable", @"") : NSLocalizedString(@"Enable", @""))];
		enabled = ([self numberOfJackConnections] == 0);
	}
	else if (action == @selector(connections:))
	{
		[menuItem setTitle:(isRunning ? [self jackConnectionsString] : NSLocalizedString(@"No connections", @""))];
		enabled = isRunning;
	}
	else if (action == @selector(cpuLoad:))
	{
		[menuItem setTitle:(isRunning ? [self jackCPULoad] : NSLocalizedString(@"No load", @""))];
		enabled = isRunning; 
	}
		
	return enabled;
}

//
#pragma mark JACK-CCESS
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

- (NSString *)jackConnectionsString
{
	NSString *s;
	int c = [self numberOfJackConnections];
	
	if (jackID != nil)
		s = [NSString stringWithFormat:NSLocalizedString(@"Connections", @""), c];
	else
		s = NSLocalizedString(@"No connections", @"");
	
	return s;
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
				port = jack_port_by_name(jackID, portName);
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
			free(ports);
		}
	}
	
	return count;
}

- (NSString *)jackCPULoad
{
	NSString *s;
	
	if (jackID != nil)
	{
		float load = jack_cpu_load(jackID);
		NSString *v;
		
		if (load < 0.5)
			v = @"<1%";
		else
			v = [NSString stringWithFormat:@"%.0f%%", load];
		s = [NSString stringWithFormat:NSLocalizedString(@"CPU Load", @""), v];
	}
	else
		s = NSLocalizedString(@"No load", @"");
	
	return s;
}

@end
