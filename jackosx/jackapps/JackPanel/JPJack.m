#import "Jack.h"
#import "JPTipView.h"
#import <Jack/jack.h>

extern jack_client_t *jack_client_new(const char *client_name) __attribute__((weak_import));

static jack_client_t *jackID;

static int jackConnectionChangedCallback(void *arg)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[(JPJack *)arg collectCommand:@"refresh" object:[NSNumber numberWithBool:YES]];
	[pool release];
	return 0;
}

static void jackPortChangedCallback(jack_port_id_t portID, int add, void *arg)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *s = [NSString stringWithCString:jack_port_name(jack_port_by_id(jackID, portID)) encoding:NSASCIIStringEncoding];

	if (add)
		[(JPJack *)arg collectCommand:@"add" object:s];
	else
		[(JPJack *)arg collectCommand:@"remove" object:s];
	
	[pool release];
}

static void jackShutdownCallback(void *arg)
{
	[NSApp performSelectorOnMainThread:@selector(terminate:) withObject:nil waitUntilDone:NO];
}

@implementation JPJack

- (id)init
{
	self = [super init];
	
	clients = [[NSMutableArray alloc] init];
	multiports = [[NSMutableArray alloc] init];
	connections = [[NSMutableArray alloc] init];
	
	collection = [[NSMutableDictionary alloc] init];
	[collection setObject:[NSMutableArray array] forKey:@"add"];
	[collection setObject:[NSMutableArray array] forKey:@"remove"];
	[collection setObject:[NSNumber numberWithBool:NO] forKey:@"refresh"];
	collectionTimer = nil;
	
	editingClient = nil;
	editingClientInputMultiports = nil;
	editingClientOutputMultiports = nil;
	
	return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
	jack_status_t status;
	const char **ports;
	char *portFullName;
	NSArray *components;
	NSString *portName, *clientName, *driverName;
	int portFlags, portType;
	JPClient *c;
	int p, i;
	
	if (jack_client_new == nil)
	{
		NSRunAlertPanel(NSLocalizedString(@"Jack Not Found Alert", @""), NSLocalizedString(@"Jack Not Found Message", @""), NSLocalizedString(@"Quit", @""), @"", @"");
		[NSApp terminate:self];
	}

	jackID = jack_client_open("JackPanel", JackNoStartServer | JackUseExactName, &status);
	
	if (jackID == nil)
	{
		if (status == JackFailure | JackServerFailed)
		{
			NSRunAlertPanel(NSLocalizedString(@"Jack Not Running Alert", @""), NSLocalizedString(@"Jack Not Running Message", @""), NSLocalizedString(@"Quit", @""), @"", @"");
			// launch from here as a convenience?
			[NSApp terminate:self];
		}
		
		// other errors to watch out for?
		// just fail with unknown error otherwise?
		// we need to continue WITH client alive
	}

	// build existing clients, assign ports
	ports = jack_get_ports(jackID, nil, nil, 0);
	if (ports != nil)
	{
		p = 0;
		while (portFullName = (char *)ports[p++])
		{
			components = [[NSString stringWithCString:portFullName encoding:NSASCIIStringEncoding] componentsSeparatedByString:@":"];
			driverName = ([components count] == 3 ? [components objectAtIndex:0] : nil);
			clientName = [components objectAtIndex:[components count] - 2];
			portName = [components lastObject];
						
			portFlags = jack_port_flags(jack_port_by_name(jackID, portFullName));
			portType = kPortTypeNone;
			if (portFlags & JackPortIsInput) portType = kPortTypeInput;
			if (portFlags & JackPortIsOutput) portType = kPortTypeOutput;
			
			c = [self clientWithName:clientName];
			if (c == nil)
			{
				c = [JPClient clientWithName:clientName driverName:driverName];
				[clients addObject:c];
			}
			[c addPortWithName:portName type:portType];
		}
		
		free(ports);
	}

	// find driver client and split it into input and output clients
	for(i=0;i<[clients count];i++)
	{
		if ([[clients objectAtIndex:i] driverName] != nil)
		{
			[clients addObject:[[clients objectAtIndex:i] splitDriver]];
			break;
		}
	}
			
	// set default placements & sizes
	for(i=0;i<[clients count];i++)
	{
		c = [clients objectAtIndex:i];
		[c setWidth:kDefaultClientWidth];

		if ([c driverName] != nil) // if it's a driver
		{
			if ([[c outputPorts] count] > 0) // if it's the input driver
				[c setOrigin:[view defaultInputDeviceOrigin]];
			else
				[c setOrigin:[view defaultOutputDeviceOrigin]];
		}
		else
			[c setOrigin:[view newClientOrigin]];
	}
		
	jack_set_port_registration_callback(jackID, jackPortChangedCallback, self);
	jack_set_graph_order_callback(jackID, jackConnectionChangedCallback, self);
	jack_on_shutdown(jackID, jackShutdownCallback, self);
	jack_activate(jackID);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *path;
	BOOL isDir;
	
	path = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportDirectory"] stringByExpandingTildeInPath];
    if ([fileManager fileExistsAtPath:path isDirectory:&isDir] == NO)
        [fileManager createDirectoryAtPath:path attributes:nil];

	path = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportSetupsDirectory"] stringByExpandingTildeInPath];
    if ([fileManager fileExistsAtPath:path isDirectory:&isDir] == NO)
        [fileManager createDirectoryAtPath:path attributes:nil];
	
	[self updateLoadSetupMenu];

	[self loadClients:[[NSUserDefaults standardUserDefaults] arrayForKey:@"clients"]];
	
	// if client has ports but no multiports to represent any of them, make a default set
	{
		NSEnumerator *e = [clients objectEnumerator];
		JPMultiport *m;
		JPClient *c;
		
		while (c = [e nextObject])
		{
			if ([[c inputPorts] count] > 0 && [[c multiportsForType:kPortTypeInput] count] == 0)
			{
				m = [JPMultiport multiport];
				[m setClient:c];
				[m setType:kPortTypeInput];
				[m addPortIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[c inputPorts] count])]];
				[multiports addObject:m];
				[c setNeedsDisplay];
			}

			if ([[c outputPorts] count] > 0 && [[c multiportsForType:kPortTypeOutput] count] == 0)
			{
				m = [JPMultiport multiport];
				[m setClient:c];
				[m setType:kPortTypeOutput];
				[m addPortIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[c outputPorts] count])]];
				[multiports addObject:m];
				[c setNeedsDisplay];
			}
		}
	}
	
	[window makeKeyAndOrderFront:self];
	[window makeFirstResponder:view];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	NSMutableArray *clientState = [NSMutableArray array];
	NSEnumerator *e;
	JPClient *c;
	
	// write client state to defaults
	e = [clients objectEnumerator];
	while (c = [e nextObject])
		[clientState addObject:[c state]];
	[[NSUserDefaults standardUserDefaults] setObject:clientState forKey:@"clients"];
	
	[collection release];
	[connections release];
	[multiports release];
	[clients release];
	jack_deactivate(jackID);
	jack_client_close(jackID);
}

- (void)updateLoadSetupMenu
{
	NSMenu *menu = [[[[[NSApp mainMenu] itemWithTitle:NSLocalizedString(@"Setup", @"")] submenu] itemWithTitle:NSLocalizedString(@"Load Setup", @"")] submenu];
	NSString *setup;
	int i;
	
	for(i=[menu numberOfItems]-1;i>=0;i--)
		[menu removeItemAtIndex:i];

	NSString *setupsPath = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportSetupsDirectory"] stringByExpandingTildeInPath];
	NSArray *setups = [[NSFileManager defaultManager] directoryContentsAtPath:setupsPath];
	
	for(i=0;i<[setups count];i++)
	{
		setup = [setups objectAtIndex:i];
		if ([[setup pathExtension] isEqualToString:@"setup"])
			[menu addItemWithTitle:[setup stringByDeletingPathExtension] action:@selector(loadSetup:) keyEquivalent:@""];
	}
	
	if ([menu numberOfItems] == 0)
		[menu addItemWithTitle:NSLocalizedString(@"<none>", @"") action:nil keyEquivalent:@""];
}

//
#pragma mark JACK-CESS
//

- (NSMutableArray *)currentJackConnections
{
	NSMutableArray *a = [NSMutableArray array];
	const char **ports, **conns;
	char *outputPortFullName, *inputPortFullName;
	jack_port_t *port;
	NSString *outputPortString, *inputPortString;
	NSArray *outputComponents, *inputComponents;
	int p, k;

	ports = jack_get_ports(jackID, nil, nil, 0);
	if (ports != nil)
	{
		p = 0;
		while (outputPortFullName = (char *)ports[p++])
		{
			port = jack_port_by_name(jackID, outputPortFullName);
			if (jack_port_flags(port) & JackPortIsOutput)
			{
				conns = jack_port_get_all_connections(jackID, port);
				if (conns != nil)
				{
					k = 0;
					while (inputPortFullName = (char *)conns[k++])
					{
						outputPortString = [NSString stringWithCString:outputPortFullName encoding:NSASCIIStringEncoding];
						outputComponents = [outputPortString componentsSeparatedByString:@":"];
						inputPortString = [NSString stringWithCString:inputPortFullName encoding:NSASCIIStringEncoding];
						inputComponents = [inputPortString componentsSeparatedByString:@":"];
						
						[a addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:
							[NSMutableDictionary dictionaryWithObjectsAndKeys:
								outputPortString, @"fullName",
								[outputComponents objectAtIndex:[outputComponents count] - 2], @"client",
								[outputComponents lastObject], @"name",
								([outputComponents count] == 3 ? [outputComponents objectAtIndex:0] : nil), @"driver",
								nil], @"output",
							[NSMutableDictionary dictionaryWithObjectsAndKeys:
								inputPortString, @"fullName",
								[inputComponents objectAtIndex:[inputComponents count] - 2], @"client",
								[inputComponents lastObject], @"name",
								([inputComponents count] == 3 ? [inputComponents objectAtIndex:0] : nil), @"driver",
								nil], @"input",
							nil]];
					}
					
					free(conns);
				}
			}
		}
		
		free(ports);
	}
	
	return a;
}

//
#pragma mark CALLBACKS
//

- (void)collectCommand:(NSString *)key object:(id)obj
{
	NSArray *components;
	NSString *driverName, *clientName, *portName;
	NSNumber *isInput;
	NSMutableArray *a;
	NSDictionary *d;
	NSEnumerator *e;
	BOOL found = NO;

	if (collectionTimer == nil)
		[self performSelectorOnMainThread:@selector(startCollectionTimer:) withObject:nil waitUntilDone:NO];
		
	if ([key isEqualToString:@"add"] || [key isEqualToString:@"remove"])
	{
		// parse out the name & info of the port
		components = [obj componentsSeparatedByString:@":"];
		driverName = ([components count] == 3 ? [components objectAtIndex:0] : nil);
		clientName = [components objectAtIndex:[components count] - 2];
		portName = [components lastObject];
		isInput = [NSNumber numberWithBool:(jack_port_flags(jack_port_by_name(jackID, [obj UTF8String])) & JackPortIsInput)];
		
		// get the array this should apply to (based on the command key given)
		a = [collection objectForKey:key];
		
		// if a client already exists for this command, simply
		// add the port (and port type) to it's list
		e = [a objectEnumerator];
		while (d = [e nextObject])
			if ([[d objectForKey:@"client"] isEqualToString:clientName])
			{
				[[d objectForKey:@"names"] addObject:portName];
				[[d objectForKey:@"areInputs"] addObject:isInput];
				found = YES;
			}
		
		// otherwise add a new entry
		if (!found)
			[a addObject:[NSDictionary dictionaryWithObjectsAndKeys:
					clientName, @"client",
					[NSMutableArray arrayWithObject:portName], @"names",
					[NSMutableArray arrayWithObject:isInput], @"areInputs",
					driverName, @"driver",
					nil]];
	}
	else if ([key isEqualToString:@"refresh"])
		[collection setObject:obj forKey:key];
}

- (void)startCollectionTimer:(id)obj
{
	// the full results seem to come back after a little over 1.0 seconds
	collectionTimer = [NSTimer scheduledTimerWithTimeInterval:1.25 target:self selector:@selector(performCollection:) userInfo:nil repeats:NO];
}

- (void)performCollection:(NSTimer *)timer
{
	NSEnumerator *e;
	NSDictionary *d;
	
	e = [[collection objectForKey:@"add"] objectEnumerator];
	while (d = [e nextObject])
		[self addPorts:d];
	
	e = [[collection objectForKey:@"remove"] objectEnumerator];
	while (d = [e nextObject])
		[self removePorts:d];
	
	if ([[collection objectForKey:@"refresh"] boolValue])
		[self updateConnections];
	
	[[collection objectForKey:@"add"] removeAllObjects];
	[[collection objectForKey:@"remove"] removeAllObjects];
	[collection setObject:[NSNumber numberWithBool:NO] forKey:@"refresh"];
	collectionTimer = nil;
}

- (void)addPorts:(NSDictionary *)d
{
	NSString *driverName = [d objectForKey:@"driver"];
	NSString *clientName = [d objectForKey:@"client"];
	NSArray *portNames = [d objectForKey:@"names"];
	NSArray *portTypes = [d objectForKey:@"areInputs"];
	JPClient *c;
	JPMultiport *m;
	int oic, ooc;
	int ic = 0, oc = 0;
	int t, i;
	
	// can't currently add ports to a driver that's running
	// so we won't worry about this just yet 
	//if (driverName != nil) c = [self driverClientForType:[portTypes objectAtIndex:0]]; else
	
	c = [self clientWithName:clientName];

	if (c == nil)
	{
		c = [JPClient clientWithName:clientName driverName:driverName];
		[c setWidth:kDefaultClientWidth];
		[c setOrigin:[view newClientOrigin]];
		[clients addObject:c];
	}
	
	oic = [[c inputPorts] count];
	ooc = [[c outputPorts] count];
	for(i=0;i<[portNames count];i++)
	{
		t = [[portTypes objectAtIndex:i] intValue];
		[c addPortWithName:[portNames objectAtIndex:i] type:t];
		(t == kPortTypeInput ? ic++ : oc++);
	}
	
	// add a new multiport for the new inputs
	if (ic > 0)
	{
		m = [JPMultiport multiport];
		[m setClient:c];
		[m setType:kPortTypeInput];
		[m addPortIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(oic, ic)]];
		[multiports addObject:m];
		[c setNeedsDisplay]; // set to set the new multiport position
	}
	
	// add a new multiport for the new outputs
	if (oc > 0)
	{
		m = [JPMultiport multiport];
		[m setClient:c];
		[m setType:kPortTypeOutput];
		[m addPortIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(ooc, ic)]];
		[multiports addObject:m];
		[c setNeedsDisplay]; // set to set the new multiport position
	}
	
	[view updateFrame];
}

- (void)removePorts:(NSDictionary *)d
{
	//NSString *driverName = [d objectForKey:@"driver"];
	NSString *clientName = [d objectForKey:@"client"];
	NSArray *portNames = [d objectForKey:@"names"];
	NSArray *portTypes = [d objectForKey:@"areInputs"];
	JPClient *c;
	int i;
	
	// can't currently add ports to a driver that's running
	// so we won't worry about this just yet 
	//if (driverName != nil) c = [self driverClientForType:[portTypes objectAtIndex:0]]; else

	c = [self clientWithName:clientName];
	
	for(i=0;i<[portNames count];i++)
		[c removePortWithName:[portNames objectAtIndex:i] type:[[portTypes objectAtIndex:i] intValue]];
}

- (void)updateConnections
{
	NSMutableArray *jackConnections = [self currentJackConnections];
	NSEnumerator *e;
	NSMutableDictionary *connectionDict, *clientDict;
	
	// NSLog(@"%@", jackConnections);
	
	// replace client/driver string objects in dictionary with clientObj they refer to
	e = [jackConnections objectEnumerator];
	while (connectionDict = [e nextObject])
	{
		clientDict = [connectionDict objectForKey:@"input"];
		if ([clientDict objectForKey:@"driver"] != nil)
			[clientDict setObject:[self driverClientForType:kPortTypeInput] forKey:@"clientObj"];
		else
			[clientDict setObject:[self clientWithName:[clientDict objectForKey:@"client"]] forKey:@"clientObj"];
		[clientDict removeObjectForKey:@"driver"];
		[clientDict removeObjectForKey:@"client"];

		clientDict = [connectionDict objectForKey:@"output"];
		if ([clientDict objectForKey:@"driver"] != nil)
			[clientDict setObject:[self driverClientForType:kPortTypeOutput] forKey:@"clientObj"];
		else
			[clientDict setObject:[self clientWithName:[clientDict objectForKey:@"client"]] forKey:@"clientObj"];
		[clientDict removeObjectForKey:@"driver"];
		[clientDict removeObjectForKey:@"client"];
	}
	
	NSMutableDictionary *anchor, *compare;
	NSMutableIndexSet *matchSet = [NSMutableIndexSet indexSet];
	JPMultiport *inputPort, *outputPort, *m;
	NSMutableArray *inputPortNames = [NSMutableArray array];
	NSMutableArray *outputPortNames = [NSMutableArray array];
	int i;
	
	// start from scratch
	[connections removeAllObjects];

	// go through all connections, look for "matches"
	while ([jackConnections count] > 0)
	{
		// clean up
		[matchSet removeAllIndexes];
		[inputPortNames removeAllObjects];
		[outputPortNames removeAllObjects];
		
		// set up
		anchor = [jackConnections objectAtIndex:0];
		[inputPortNames addObject:[[anchor objectForKey:@"input"] objectForKey:@"name"]];
		[outputPortNames addObject:[[anchor objectForKey:@"output"] objectForKey:@"name"]];
		
		for(i=1;i<[jackConnections count];i++) // compare rest against first object in list
		{
			compare = [jackConnections objectAtIndex:i];
			
			// input and output clients must be the same
			if ([[anchor objectForKey:@"input"] objectForKey:@"clientObj"] == [[compare objectForKey:@"input"] objectForKey:@"clientObj"] &&
				[[anchor objectForKey:@"output"] objectForKey:@"clientObj"] == [[compare objectForKey:@"output"] objectForKey:@"clientObj"])
			{
				// must have exclusive ports
				if (![inputPortNames containsObject:[[compare objectForKey:@"input"] objectForKey:@"name"]] &&
					![outputPortNames containsObject:[[compare objectForKey:@"output"] objectForKey:@"name"]])
				{
					// add this connection's index and add the port names to the list
					[matchSet addIndex:i];
					[inputPortNames addObject:[[compare objectForKey:@"input"] objectForKey:@"name"]];
					[outputPortNames addObject:[[compare objectForKey:@"output"] objectForKey:@"name"]];
				}
			}
		}
		
		// create input multiport
		inputPort = [JPMultiport multiport];
		[inputPort setClient:[[anchor objectForKey:@"input"] objectForKey:@"clientObj"]];
		[inputPort setType:kPortTypeInput];
		[inputPort addPortNames:inputPortNames];
		
		// look for a multiport that matches the input port
		if (m = [self multiportMatchingMultiport:inputPort])
			inputPort = m;
		else
		{
			[multiports addObject:inputPort];
			[[[anchor objectForKey:@"input"] objectForKey:@"clientObj"] setNeedsDisplay];
		}
		
		// create output port
		outputPort = [JPMultiport multiport];
		[outputPort setClient:[[anchor objectForKey:@"output"] objectForKey:@"clientObj"]];
		[outputPort setType:kPortTypeOutput];
		[outputPort addPortNames:outputPortNames];

		// look for a multiport that matches the input port
		if (m = [self multiportMatchingMultiport:outputPort])
			outputPort = m;
		else
		{
			[multiports addObject:outputPort];
			[[[anchor objectForKey:@"output"] objectForKey:@"clientObj"] setNeedsDisplay];
		}
		
		// add the anchor to the set to be removed
		[matchSet addIndex:0];
		[jackConnections removeObjectsAtIndexes:matchSet];
		
		// add the new connection
		[connections addObject:[JPConnection connectionWithInputPort:inputPort outputPort:outputPort]];
	}
	
	[view setNeedsDisplay:YES]; // has to be everything, the connections don't yet know their frames
}

//
#pragma mark OBJECT ACTIONS
//

- (void)loadClients:(NSArray *)a
{
	NSEnumerator *e = [a objectEnumerator];
	NSDictionary *d;
	JPClient *c;
	id obj;
	
	while (d = [e nextObject])
	{
		if ([d objectForKey:@"driverName"] != nil)
			c = [self driverClientForType:[[d objectForKey:@"driverIsInput"] intValue]];
		else
			c = [self clientWithName:[d objectForKey:@"name"]];
		
		if (c == nil)
		{
			c = [JPClient clientWithName:[d objectForKey:@"name"] driverName:[d objectForKey:@"driverName"]];
			[clients addObject:c];
		}
		
		obj = [d objectForKey:@"origin"];
		if (obj) [c setOrigin:NSPointFromString(obj)];
		
		obj = [d objectForKey:@"width"];
		if (obj) [c setWidth:[obj floatValue]];
		
		obj = [d objectForKey:@"multiports"];
		if (obj && [c hasPorts])
		{
			NSEnumerator *n = [obj objectEnumerator];
			NSDictionary *t;
			JPMultiport *m;
			
			while (t = [n nextObject])
			{
				m = [JPMultiport multiport];
				[m setClient:c];
				[m setType:[[t objectForKey:@"type"] intValue]];
				[m setName:[t objectForKey:@"name"]];
				[m addPortNames:[t objectForKey:@"ports"]];
				if ([self multiportMatchingMultiport:m] == nil)
					[multiports addObject:m];
			}
		}
		
		[c setNeedsDisplay];
	}
	
	[view updateFrame];
}

- (void)loadConnections:(NSArray *)a
{
	NSEnumerator *e;
	NSDictionary *c;
	int err;
	
	e = [[self currentJackConnections] objectEnumerator];
	while (c = [e nextObject])
	{
		err = jack_disconnect(jackID, [[[c objectForKey:@"output"] objectForKey:@"fullName"] UTF8String], [[[c objectForKey:@"input"] objectForKey:@"fullName"] UTF8String]);
	}
	
	e = [a objectEnumerator];
	while (c = [e nextObject])
	{
		err = jack_connect(jackID, [[c objectForKey:@"output"] UTF8String], [[c objectForKey:@"input"] UTF8String]);
		// alert user of errors? not really a big deal...
	}
}

- (void)connectMultiport:(JPMultiport *)outputMultiport toMultiport:(JPMultiport *)inputMultiport
{
	JPMultiport *m;
	NSArray *inputPortNames, *outputPortNames;
	int i, err;
	
	if ([outputMultiport type] == kPortTypeInput)
		{ m = outputMultiport; outputMultiport = inputMultiport; inputMultiport = m; }
	
	inputPortNames = [inputMultiport portFullNames];
	outputPortNames = [outputMultiport portFullNames];
	
	for(i=0;i<[inputPortNames count];i++)
		err = jack_connect(jackID, [[outputPortNames objectAtIndex:i] UTF8String], [[inputPortNames objectAtIndex:i] UTF8String]);
	
	[connections addObject:[JPConnection connectionWithInputPort:inputMultiport outputPort:outputMultiport]];
	[view setNeedsDisplay:YES]; // don't know it's frame yet
}

- (void)removeConnections:(NSArray *)c
{
	NSEnumerator *e = [c objectEnumerator];
	JPConnection *conn;
	NSArray *inputPortNames, *outputPortNames;
	int i, err;
	
	while (conn = [e nextObject])
	{
		inputPortNames = [[conn inputPort] portFullNames];
		outputPortNames = [[conn outputPort] portFullNames];
		
		for(i=0;i<[inputPortNames count];i++)
			err = jack_disconnect(jackID, [[outputPortNames objectAtIndex:i] UTF8String], [[inputPortNames objectAtIndex:i] UTF8String]);
		
		[conn setNeedsDisplay];
	}
	
	[connections removeObjectsInArray:c];
}

- (void)removeClients:(NSArray *)c
{
	JPClient *client;
	int i;

	for(i=[clients count]-1;i>=0;i--)
	{
		client = [clients objectAtIndex:i];
		if ([c containsObject:client] && [[client inputPorts] count] == 0 && [[client outputPorts] count] == 0)
		{
			[client setNeedsDisplay];
			[clients removeObject:client];
		}
	}
}

- (void)bringClientToFront:(JPClient *)c
{
	int i = [clients indexOfObjectIdenticalTo:c];
	[clients insertObject:c atIndex:0];
	[clients removeObjectAtIndex:i + 1];
	
	[c setNeedsDisplay];
}

- (void)clearAllSelections
{
	[self clearClientSelections];
	[self clearConnectionSelections];
}

- (void)clearClientSelections
{
	[clients makeObjectsPerformSelector:@selector(clearSelection)];
}

- (void)clearConnectionSelections
{
	[connections makeObjectsPerformSelector:@selector(clearSelection)];
}

//
#pragma mark MENUS
//

- (IBAction)loadSetup:(id)sender
{
	NSString *setupPath = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportSetupsDirectory"] stringByExpandingTildeInPath];
	NSDictionary *setup;
	
	setupPath = [[setupPath stringByAppendingPathComponent:[sender title]] stringByAppendingPathExtension:@"setup"];
	setup = [NSDictionary dictionaryWithContentsOfFile:setupPath];
	
	[self loadClients:[setup objectForKey:@"clients"]];
	[self loadConnections:[setup objectForKey:@"connections"]];
	
	[view setNeedsDisplay:YES];
}

- (IBAction)saveSetup:(id)sender
{
	[NSApp beginSheet:saveSetupPanel modalForWindow:window modalDelegate:self didEndSelector:@selector(saveSetupPanelDidClose:returnCode:contextInfo:) contextInfo:nil];
}

- (IBAction)closeSaveSetupPanel:(id)sender
{
	NSString *name = [saveSetupNameText stringValue];
	NSString *setupPath = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportSetupsDirectory"] stringByExpandingTildeInPath];
	
	if ([sender tag])
	{
		if ([name length] == 0) { NSBeep(); return; }
		
		setupPath = [[setupPath stringByAppendingPathComponent:name] stringByAppendingPathExtension:@"setup"];

		if ([[NSFileManager defaultManager] fileExistsAtPath:setupPath])
			if (NSRunAlertPanel([NSString stringWithFormat:NSLocalizedString(@"Setup Exists Title", @""), name], NSLocalizedString(@"Setup Exists Message", @""), NSLocalizedString(@"Replace", @""), @"", NSLocalizedString(@"Cancel", @"")) != NSAlertDefaultReturn) return;
		
		[saveSetupPanel makeFirstResponder:saveSetupPanel];
	}
	
	[NSApp endSheet:saveSetupPanel returnCode:[sender tag]];
}

- (void)saveSetupPanelDidClose:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode)
	{
		NSMutableDictionary *setup = [NSMutableDictionary dictionary];
		NSMutableArray *clientState = [NSMutableArray array];
		NSMutableArray *connectionState = [NSMutableArray array];
		NSEnumerator *e;
		JPClient *c;
		JPConnection *k;
		
		e = [clients objectEnumerator];
		while (c = [e nextObject])
			[clientState addObject:[c state]];
		[setup setObject:clientState forKey:@"clients"];
			
		e = [connections objectEnumerator];
		while (k = [e nextObject])
			[connectionState addObjectsFromArray:[k state]];
		[setup setObject:connectionState forKey:@"connections"];
		
		NSString *name = [saveSetupNameText stringValue];
		NSString *setupPath = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"SupportSetupsDirectory"] stringByExpandingTildeInPath];
		
		setupPath = [[setupPath stringByAppendingPathComponent:name] stringByAppendingPathExtension:@"setup"];
		
		[setup writeToFile:setupPath atomically:NO];
		[self updateLoadSetupMenu];
	}
	
	[saveSetupPanel orderOut:self];
}

- (IBAction)editSetups:(id)sender
{
	NSLog(@"edit setups");
}

- (IBAction)openJackControl:(id)sender
{
	NSString *path;
	
	path = @"/Library/PreferencePanes/Jack.prefPane";
	if (![[NSFileManager defaultManager] fileExistsAtPath:path])
		path = @"~/Library/PreferencePanes/Jack.prefPane";
	
	[[NSWorkspace sharedWorkspace] openFile:[path stringByExpandingTildeInPath]]; 
}

- (IBAction)openAudioMIDISetup:(id)sender
{
    [[NSWorkspace sharedWorkspace] launchApplication:@"Audio MIDI Setup"];
}

//
#pragma mark ACCESS
//

- (NSArray *)clients
{
	return clients;
}

- (NSArray *)selectedClients
{
	NSMutableArray *a = [NSMutableArray array];
	NSEnumerator *e = [clients objectEnumerator];
	JPClient *c;

	while (c = [e nextObject])
		if ([c selected])
			[a addObject:c];
	
	return a;
}

- (JPClient *)clientWithName:(NSString *)n
{
	NSEnumerator *e = [clients objectEnumerator];
	JPClient *c, *foundClient = nil;
	
	while ((c = [e nextObject]) && !foundClient)
		if ([[c name] isEqualToString:n])
				foundClient = c;
	
	return foundClient;
}

- (JPClient *)driverClientForType:(int)t
{
	NSEnumerator *e = [clients objectEnumerator];
	JPClient *c, *foundClient = nil;
	
	while ((c = [e nextObject]) && !foundClient)
		if ([c driverName] != nil)
		{
			if (t == kPortTypeInput && [[c inputPorts] count] > 0)
				foundClient = c;
			else if (t == kPortTypeOutput && [[c outputPorts] count] > 0)
				foundClient = c;
		}
	
	return foundClient;
}

- (NSMutableArray *)multiports
{
	return multiports;
}

- (JPMultiport *)multiportMatchingMultiport:(JPMultiport *)p
{
	JPMultiport *m = nil, *n;
	int i;
	
	for(i=0;i<[multiports count] && !m;i++)
	{
		n = [multiports objectAtIndex:i];
		if ([p isEqualToMultiport:n]) m = n;
	}
	
	return m;
}

- (NSMutableArray *)connections
{
	return connections;
}

- (NSArray *)selectedConnections
{
	NSMutableArray *a = [NSMutableArray array];
	NSEnumerator *e = [connections objectEnumerator];
	JPConnection *c;

	while (c = [e nextObject])
		if ([c selected])
			[a addObject:c];
	
	return a;
}

- (JPView *)view
{
	return view;
}

- (JPTipView *)tipView
{
	return tipView;
}

- (NSWindow *)window
{
	return window;
}

@end
