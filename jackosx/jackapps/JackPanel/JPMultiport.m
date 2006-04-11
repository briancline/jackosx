#import "Jack.h"
#import "JPTipView.h"
#import <AFExtensions/NSImage (AFExtension).h>

@implementation JPMultiport

- (id)init
{
	self = [super init];
		
	client = nil;
	type = kPortTypeNone;
	name = @"Multiport";
	ports = [[NSMutableIndexSet alloc] init];
	
	origin = NSZeroPoint;
	single = [NSImage imageNamed:@"port_single"];
	multiple = [NSImage imageNamed:@"port_multiple"];

	selected = NO;
	
	return self;
}

- (void)dealloc
{
	[ports release];
	[super dealloc];
}

- (id)initForClient:(JPClient *)c portType:(int)t
{
	unsigned int portCount;
	
	self = [self init];
	
	client = c;
	type = t;
	
	// by default, refer to ALL ports for the respective kind
	portCount = (t == kPortTypeInput ? [[c inputPorts] count] : [[c outputPorts] count]);
	[ports addIndexesInRange:NSMakeRange(0, portCount)];
	
	return self;
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"Multiport for client '%@' %@: %@", [client name], (type == kPortTypeInput ? @"input" : @"output"), ports];
}

- (BOOL)isEqualToMultiport:(JPMultiport *)p
{
	return ([p client] == client && [p type] == type && [ports isEqualToIndexSet:[p portIndexes]]);
}

//
#pragma mark CLASS METHODS
//

+ (id)multiport
{
	return [[[JPMultiport alloc] init] autorelease];
}

+ (id)multiportForClient:(JPClient *)c portType:(int)t
{
	return [[[JPMultiport alloc] initForClient:c portType:t] autorelease];
}

//
#pragma mark MOUSE
//

- (void)drag:(NSEvent *)event
{
    NSWindow *window = [Jack window];
	JPView *view = [Jack view];
	JPTipView *tipView = [Jack tipView];
	NSMutableArray *connections = [Jack connections];
	JPConnection *dragConnection, *c;
	NSMutableArray *connectableMultiports;
	NSEnumerator *e, *f;
	JPMultiport *m, *n;
	NSPoint curPoint, lastPoint = [view convertPoint:[event locationInWindow] fromView:nil];
	BOOL found;
	int i;
	
	[Jack bringClientToFront:client];
	
	// make list of connectable multiports and their frames
	connectableMultiports = [NSMutableArray array];
	e = [[Jack multiports] objectEnumerator];
	while (m = [e nextObject])
	{
		// see if multiport has the same port count as us (must match)
		// and has the opposite port type
		if ([m portCount] == [ports count] && [m type] == 1 - type)
		{
			// make sure a connection between the two doesn't already exist
			found = NO;
			f = [connections objectEnumerator];
			while ((c = [f nextObject]) && !found)
				if (([c inputPort] == self || [c outputPort] == self) && ([c inputPort] == m || [c outputPort] == m))
					found = YES;
			
			if (!found)
				[connectableMultiports addObject:m];
		}
	}
	
	// make partial connection, add it to the list
	if (type == kPortTypeInput)
		dragConnection = [JPConnection connectionWithInputPort:self outputPort:nil];
	else
		dragConnection = [JPConnection connectionWithInputPort:nil outputPort:self];
	[connections addObject:dragConnection];

	// show the tip
	[tipView setPosition:lastPoint];
	[tipView setText:[NSString stringWithFormat:@"%@\n%d %@%@", [client name], [ports count], (type == kPortTypeInput ? @"input" : @"output"), ([ports count] > 1 ? @"s" : @"")]];
	
	while (1)
	{
		event = [window nextEventMatchingMask:(NSLeftMouseDraggedMask | NSLeftMouseUpMask)];
		curPoint = [view convertPoint:[event locationInWindow] fromView:nil];	
		
		if (!NSEqualPoints(curPoint, lastPoint) && [event type] == NSLeftMouseDragged)
		{
			[dragConnection setNeedsDisplay];
			[tipView setPosition:curPoint];
			
			m = nil;
			for(i=0;i<[connectableMultiports count] && !m;i++)
			{
				n = [connectableMultiports objectAtIndex:i];
				if (NSMouseInRect(curPoint, [n frame], NO))
				{
					m = n;
					[n setSelected:YES];
				}
				else
					[n setSelected:NO];
			}
			
			lastPoint = curPoint;
		}

		if ([event type] == NSLeftMouseUp)
			break;
	}
	
	// erase the tip
	[tipView setText:nil];
	
	// remove the fake connection
	[dragConnection setNeedsDisplay];
	[connections removeObject:dragConnection];
	
	// make the real connection if one was made
	if (m != nil)
	{
		[m setSelected:NO];
		[Jack connectMultiport:self toMultiport:m];
	}
}

//
#pragma mark DRAW
//

- (void)draw
{
	NSRect srcRect = [single bounds];
	NSPoint dstPoint = origin;
		
	srcRect.size.width--;
	if (type == kPortTypeOutput)
		srcRect.origin.x++;
	
	if ([ports count] == 1)
		[single drawAtPoint:dstPoint fromRect:srcRect operation:NSCompositeSourceOver fraction:1.0];
	else
		[multiple drawAtPoint:dstPoint fromRect:srcRect operation:NSCompositeSourceOver fraction:1.0];

	if (selected)
	{
		[[NSColor colorWithCalibratedWhite:0.0 alpha:0.5] set];
		NSRectFillUsingOperation(NSMakeRect(dstPoint.x, dstPoint.y, kPortSize, kPortSize), NSCompositeSourceAtop);
	}
}

//
#pragma mark ACCESS
//

- (JPClient *)client
{
	return client;
}

- (void)setClient:(JPClient *)c
{
	client = c;
}

- (int)type
{
	return type;
}

- (void)setType:(int)t
{
	type = t;
}

- (NSString *)name
{
	return name;
}

- (void)setName:(NSString *)n
{
	[name release];
	name = [n retain];
}

- (unsigned int)portCount
{
	return [ports count];
}

- (NSArray *)portNames
{
	NSMutableArray *list = [NSMutableArray array];
	int p = [ports firstIndex];
	
	while (p != NSNotFound)
	{
		[list addObject:[client portNameAtIndex:p type:type]];
		p = [ports indexGreaterThanIndex:p];
	}

	return list;
}

- (NSArray *)portFullNames
{
	NSMutableArray *list = [NSMutableArray array];
	int p = [ports firstIndex];
	
	while (p != NSNotFound)
	{
		[list addObject:[client portFullNameAtIndex:p type:type]];
		p = [ports indexGreaterThanIndex:p];
	}

	return list;
}

- (NSIndexSet *)portIndexes
{
	return ports;
}

- (BOOL)referencesPort:(NSString *)p
{
	int i = [ports firstIndex];
	BOOL found = NO;
	
	while (i != NSNotFound && !found)
	{
		if ([p isEqualToString:[client portNameAtIndex:i type:type]])
			found = YES;
		i = [ports indexGreaterThanIndex:i];
	}
	
	return found;
}

- (BOOL)referencesPortIndex:(unsigned int)i
{
	return ([ports containsIndex:i]);
}

- (void)setPortIndex:(unsigned int)i state:(BOOL)s
{	
	if ([self hasConnections]) return;
	
	if (s)
		[ports addIndex:i];
	else
	{
		if (!([ports count] == 1 && i == [ports firstIndex])) // need to have at least one port
			[ports removeIndex:i];
	}
	
	[[Jack view] setNeedsDisplayInRect:[self frame]];
}

- (void)addPortIndexes:(NSIndexSet *)s
{	
	if ([self hasConnections]) return;

	[ports addIndexes:s];
	[[Jack view] setNeedsDisplayInRect:[self frame]];
}

- (void)addPortNames:(NSArray *)a
{
	if ([self hasConnections]) return;
	
	NSEnumerator *e = [a objectEnumerator];
	NSString *p;
	unsigned int i;
	
	while (p = [e nextObject])
	{
		i = [client indexOfPort:p type:type];
		if (i != NSNotFound) [ports addIndex:i];
	}
	
	[[Jack view] setNeedsDisplayInRect:[self frame]];
}

- (void)removePort:(NSString *)p
{
	if ([self hasConnections]) return;
	
	unsigned int i = [client indexOfPort:p type:type];
	
	if (i != NSNotFound)
		[ports shiftIndexesStartingAtIndex:i + 1 by:-1];
}

- (NSPoint)origin
{
	return origin;
}

- (void)setOrigin:(NSPoint)p
{
	origin = p;
}

- (NSRect)frame
{
	return NSMakeRect(origin.x, origin.y, kPortSize, kPortSize);
}

- (BOOL)selected
{
	return selected;
}

- (void)setSelected:(BOOL)s
{
	if (s != selected)
		[[Jack view] setNeedsDisplayInRect:[self frame]]; // needs display
	selected = s;
}

- (BOOL)hasConnections
{
	NSEnumerator *e = [[Jack connections] objectEnumerator];
	JPConnection *c;
	BOOL connected = NO;
	
	while ((c = [e nextObject]) && !connected)
		if ([c inputPort] == self || [c outputPort] == self)
			connected = YES;
	
	return connected;
}

- (NSArray *)connections
{
	NSEnumerator *e = [[Jack connections] objectEnumerator];
	NSMutableArray *a = [NSMutableArray array];
	JPConnection *c;
	
	while (c = [e nextObject])
		if ([c inputPort] == self || [c outputPort] == self)
			[a addObject:c];
	
	return a;
}

@end
