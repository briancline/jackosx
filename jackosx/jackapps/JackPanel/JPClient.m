#import "Jack.h"
#import <AFExtensions/NSImage (AFExtension).h>

@implementation JPClient

- (id)init
{
	self = [super init];
	
	inputPorts = [[NSMutableArray alloc] init];
	outputPorts = [[NSMutableArray alloc] init];
	
	name = nil;
	driverName = nil;
	origin = NSZeroPoint;
	image = [[NSImage alloc] initWithSize:NSZeroSize];
	needsRefresh = YES;
	
	underlay = [NSImage imageNamed:@"underlay"];
	gradient = [NSImage imageNamed:@"gradient"];
	overlay = [NSImage imageNamed:@"overlay"];
	resize = [NSImage imageNamed:@"resize"];
	icon = nil;
	iconShadow = [[NSShadow alloc] init];
	[iconShadow setShadowOffset:NSMakeSize(3, -3)];
	[iconShadow setShadowBlurRadius:3];
	color = [[NSColor colorWithCalibratedHue:0.583 saturation:0.7 brightness:1.0 alpha:1.0] retain];
	
	NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle alloc] init] autorelease];
	[style setAlignment:NSCenterTextAlignment];
	[style setLineBreakMode:NSLineBreakByTruncatingTail];
	titleAttributes = [[NSDictionary alloc] initWithObjectsAndKeys:
						[NSColor whiteColor], NSForegroundColorAttributeName,
						[NSFont systemFontOfSize:[NSFont smallSystemFontSize]], NSFontAttributeName,
						style, NSParagraphStyleAttributeName,
						nil];
	
	selected = NO;
	
	return self;
}

- (id)initWithName:(NSString *)n driverName:(NSString *)d
{	
	self = [self init];
	
	name = [n retain];
	driverName = [d retain];
	
	if (driverName == nil) // is an application, has a real icon somewhere
	{
		NSString *path;
		NSBundle *bundle;

		if (path = [[NSWorkspace sharedWorkspace] fullPathForApplication:name])
		{		
			if (bundle = [NSBundle bundleWithPath:path])
			{
				NSString *iconPath = [bundle pathForResource:[[bundle objectForInfoDictionaryKey:@"CFBundleIconFile"] stringByDeletingPathExtension] ofType:@"icns"];
				icon = [[NSImage alloc] initByReferencingFile:iconPath];
			}
			else
			{			
				CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)path, kCFURLPOSIXPathStyle, NO);
				NSDictionary *infoDict = (NSDictionary *)CFBundleCopyInfoDictionaryForURL(url);
				NSString *icnsResID = [infoDict objectForKey:@"CFBundleIconFile"];
				FSRef ref;
				short resFile;
				Handle icnsHandle;
				NSData *icnsData;
				
				if (FSPathMakeRef((const UInt8 *)[path fileSystemRepresentation], &ref, nil) == noErr)
				{
					resFile = FSOpenResFile(&ref, fsRdPerm);
					if (ResError() == noErr)
					{
						if (icnsHandle = Get1Resource('icns', [icnsResID intValue]))
						{
							if (icnsData = [NSData dataWithBytes:*icnsHandle length:GetHandleSize(icnsHandle)])
							icon = [[NSImage alloc] initWithData:icnsData];
						}
						CloseResFile(resFile);
					}
				}
			}
		}
	}
			
	return self;
}

- (void)dealloc
{
	[iconShadow release];
	[color release];
	[icon release];
	[image release];
	[name release];
	[driverName release];
	[inputPorts release];
	[outputPorts release];
	[super dealloc];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"'%@', %d input ports, %d output ports", name, [inputPorts count], [outputPorts count]];
}

- (JPClient *)splitDriver
{
	JPClient *inputDriverClient = [[[JPClient alloc] initWithName:name driverName:driverName] autorelease];
	
	[[inputDriverClient outputPorts] addObjectsFromArray:outputPorts];
	[outputPorts removeAllObjects];
	[self setIcon:[NSImage imageNamed:@"icon_driver_output"]];
	[inputDriverClient setIcon:[NSImage imageNamed:@"icon_driver_input"]];
	
	return inputDriverClient;
}

//
#pragma mark CLASS METHODS
//

+ (id)clientWithName:(NSString *)n driverName:(NSString *)d
{
	return [[[JPClient alloc] initWithName:n driverName:d] autorelease];
}

//
#pragma mark PORTS
//

- (void)addPortWithName:(NSString *)p type:(int)t
{
	NSMutableArray *a = [self portsOfType:t];
	BOOL wasInactive = ([inputPorts count] == 0 && [outputPorts count] == 0);
	
	if ([a indexOfObject:p] == NSNotFound)
		[a addObject:p];
	
	if (wasInactive) needsRefresh = YES;
	[self setNeedsDisplay];
}

- (void)removePortWithName:(NSString *)p type:(int)t
{
	NSMutableArray *a = [self portsOfType:t];
	unsigned int portIndexToRemove;
	NSArray *multiports;
	NSEnumerator *e;
	JPMultiport *m;
	
	portIndexToRemove = [a indexOfObject:p];
	
	multiports = [self multiportsForPort:p];
	e = [multiports objectEnumerator];
	while (m = [e nextObject])
	{
		[[Jack connections] removeObjectsInArray:[m connections]];
		[m removePort:p];
		if ([m portCount] == 0)
		{
			[self setNeedsDisplay];
			[[Jack multiports] removeObject:m];
		}
	}

	[a removeObjectAtIndex:portIndexToRemove];
	if ([inputPorts count] == 0 && [outputPorts count] == 0)
	{
		needsRefresh = YES;
		[self setNeedsDisplay];
	}
}

//
#pragma mark DRAWING
//

- (void)draw:(NSRect)updateRect
{	
	NSEnumerator *e = [[self multiports] objectEnumerator];
	JPMultiport *m;
	NSPoint o;
	
	// draw multiports for this client in order
	while (m = [e nextObject])
		[m draw];

	// rebuild body image if needed
	if (needsRefresh)
	{
		NSRect bounds = NSMakeRect(0, 0, width, height + kTitleBarHeight);
		NSRect titleBarRect = NSMakeRect(0, NSMaxY(bounds) - kTitleBarHeight, width, kTitleBarHeight);
		BOOL active = [self hasPorts];
		[image setSize:bounds.size];
		[image lockFocus];
		
		[[NSColor clearColor] set];
		NSRectFill(bounds);
		
		[underlay sliceScaleToRect:bounds operation:NSCompositeSourceOver fraction:1.0];

		[[color blendedColorWithFraction:(active ? 0.0 : 0.5) ofColor:[NSColor whiteColor]] set];
		if (selected)
			NSRectFillUsingOperation(bounds, NSCompositeSourceAtop);
		else
		{
			[gradient drawInRect:NSMakeRect(0, 0, NSWidth(bounds), NSHeight(bounds) - kTitleBarHeight) fromRect:[gradient bounds] operation:NSCompositeSourceAtop fraction:1.0];
			NSRectFillUsingOperation(titleBarRect, NSCompositeSourceAtop);
		}
		[overlay sliceScaleToRect:bounds operation:NSCompositeSourceOver fraction:1.0];
		[name drawInRect:NSOffsetRect(NSInsetRect(titleBarRect, 16, 0), 0, -2) withAttributes:titleAttributes];
		[resize compositeToPoint:NSMakePoint(NSWidth(bounds) - [resize size].width - 5, 4) operation:NSCompositeSourceOver];
		if (active) [iconShadow set];
		[icon drawInRect:NSMakeRect(width * 0.1, ((NSHeight(bounds) - kTitleBarHeight) / 2) - (width * 0.4), width * 0.8, width * 0.8) fromRect:[icon bounds] operation:NSCompositeSourceOver fraction:(active ? 1.0 : 0.5)];

		[image unlockFocus];
		needsRefresh = NO;
	}
	
	// slap body image on screen
	o = origin; o.y += height;
	[image compositeToPoint:o operation:NSCompositeSourceOver];
}

- (void)setNeedsDisplay
{
	NSEnumerator *e;
	JPMultiport *m;
	JPConnection *c;
	JPView *v = [Jack view];
	int ic = 0, oc = 0;
	float h;
	
	// set origins for multiports
	// and set needs display
	e = [[self multiports] objectEnumerator];
	while (m = [e nextObject])
	{
		if ([m type] == kPortTypeInput)
			[m setOrigin:NSMakePoint(origin.x - kPortSize, origin.y + (kPortSize * ic++))];
		else
			[m setOrigin:NSMakePoint(origin.x + width, origin.y + (kPortSize * oc++))];
		[v setNeedsDisplayInRect:[m frame]];
	}
	
	// set needs display for body
	h = MAX(width, kPortSize * MAX(ic + 1, oc + 1));
	if (h != height) needsRefresh = YES;
	height = h;
	[v setNeedsDisplayInRect:NSMakeRect(origin.x, origin.y - kTitleBarHeight, width, height + kTitleBarHeight)];
		
	// set connections connected to multiports associated with self needs display
	e = [[self connections] objectEnumerator];
	
	while (c = [e nextObject])
		[v setNeedsDisplayInRect:[c frame]];
}

//
#pragma mark MOUSING
//

- (BOOL)mouseDown:(NSEvent *)event
{
	NSRect frame = NSMakeRect(origin.x, origin.y - kTitleBarHeight, width, height + kTitleBarHeight);
    NSPoint clickPoint = [[Jack view] convertPoint:[event locationInWindow] fromView:nil];
	BOOL extendSelection = ([event modifierFlags] & NSCommandKeyMask) || ([event modifierFlags] & NSShiftKeyMask);
	BOOL handled = NO;

	if (NSMouseInRect(clickPoint, frame, NO))
	{
		handled = YES;
		[Jack clearConnectionSelections];
		if (!extendSelection && !selected) [Jack clearAllSelections];
		[self setSelected:YES];
		if ([event clickCount] > 1)
			[Jack openEditorForClient:self];
		else if (NSMouseInRect(clickPoint, NSMakeRect(NSMaxX(frame) - 12, NSMaxY(frame) - 12, 12, 12), NO))
			[self resize:event];
		else	
			[self drag:event];
	}
	else
	{
		NSEnumerator *e = [[self multiports] objectEnumerator];
		JPMultiport *m;

		while ((m = [e nextObject]) && !handled)
		{
			if (NSMouseInRect(clickPoint, [m frame], NO))
				{ [m drag:event]; handled = YES; }
		}
	}
	
	return handled;
}

- (void)drag:(NSEvent *)event
{
    NSWindow *window = [Jack window];
	JPView *view = [Jack view];
	NSArray *clients = [Jack selectedClients];
	NSPoint curPoint, lastPoint = [view convertPoint:[event locationInWindow] fromView:nil];
	int i;
	
	[Jack bringClientToFront:self];

	while (1)
	{
		event = [window nextEventMatchingMask:(NSLeftMouseDraggedMask | NSLeftMouseUpMask)];
		curPoint = [view convertPoint:[event locationInWindow] fromView:nil];	
		
		if (!NSEqualPoints(curPoint, lastPoint) && [event type] == NSLeftMouseDragged)
		{
			for(i=0;i<[clients count];i++)
				[[clients objectAtIndex:i] offsetOrigin:NSMakeSize(curPoint.x - lastPoint.x, curPoint.y - lastPoint.y)];
			[view updateFrame];
			lastPoint = curPoint;
		}

		if ([event type] == NSLeftMouseUp)
			break;
	}
}

- (void)resize:(NSEvent *)event
{
    NSWindow *window = [Jack window];
	JPView *view = [Jack view];
    NSPoint curPoint, lastPoint = [view convertPoint:[event locationInWindow] fromView:nil];
	NSPoint offset = NSMakePoint(lastPoint.x - (origin.x + width), lastPoint.y - (origin.y - width));
		
	[Jack bringClientToFront:self];

	while (1)
	{
		event = [window nextEventMatchingMask:(NSLeftMouseDraggedMask | NSLeftMouseUpMask)];
		curPoint = [view convertPoint:[event locationInWindow] fromView:nil];	
		
		if (!NSEqualPoints(curPoint, lastPoint) && [event type] == NSLeftMouseDragged)
		{
			[self setWidth:MAX(curPoint.x - offset.x - origin.x, curPoint.y - offset.y - origin.y)];
			[view updateFrame];
			lastPoint = curPoint;
		}

		if ([event type] == NSLeftMouseUp)
			break;
	}
}

//
#pragma mark ACCESS
//

- (NSDictionary *)state
{
	NSEnumerator *e = [[self multiports] objectEnumerator];
	JPMultiport *m;
	NSMutableArray *multiports = [NSMutableArray array];
	
	while (m = [e nextObject])
		[multiports addObject:[NSDictionary dictionaryWithObjectsAndKeys:
								[m name], @"name",
								[NSNumber numberWithInt:[m type]], @"type",
								[m portNames], @"ports",
								nil]];
	
	return [NSDictionary dictionaryWithObjectsAndKeys:
				name, @"name",
				NSStringFromPoint(origin), @"origin",
				[NSNumber numberWithFloat:width], @"width",
				multiports, @"multiports",
				driverName, @"driverName",
				[NSNumber numberWithBool:([inputPorts count] > 0)], @"driverIsInput",
				nil];
}

- (BOOL)hasPorts
{
	return ([inputPorts count] > 0 || [outputPorts count] > 0);
}

- (NSMutableArray *)inputPorts
{
	return inputPorts;
}

- (NSMutableArray *)outputPorts
{
	return outputPorts;
}

- (NSMutableArray *)portsOfType:(int)t
{
	NSMutableArray *a;
	
	if (t == kPortTypeInput)
		a = inputPorts;
	if (t == kPortTypeOutput)
		a = outputPorts;
	
	return a;
}

- (NSString *)portNameAtIndex:(unsigned int)i type:(int)t
{
	NSMutableArray *a = [self portsOfType:t];
	NSString *s = nil;
	
	if (i < [a count])
		s = [a objectAtIndex:i];
	
	return s;
}

- (NSString *)portFullNameAtIndex:(unsigned int)i type:(int)t
{
	NSMutableArray *a = [self portsOfType:t];
	NSString *s = nil;
	
	if (i < [a count])
		s = [NSString stringWithFormat:@"%@%@:%@",
					(driverName != nil ? [NSString stringWithFormat:@"%@:", driverName] : @""),
					name, [a objectAtIndex:i]];
	
	return s;
}

- (unsigned int)indexOfPort:(NSString *)p type:(int)t
{
	NSMutableArray *a = [self portsOfType:t];
	
	return [a indexOfObject:p];
}

- (NSString *)name
{
	return name;
}

- (NSString *)driverName
{
	return driverName;
}

- (NSPoint)origin
{
	return origin;
}

- (NSImage *)icon
{
	return icon;
}

- (void)setOrigin:(NSPoint)p
{
	[self setNeedsDisplay];
	origin.x = MAX(p.x, kPortSize);
	origin.y = MAX(p.y, kTitleBarHeight);
	[self setNeedsDisplay];
}

- (void)offsetOrigin:(NSSize)s
{
	[self setOrigin:NSMakePoint(origin.x + s.width, origin.y + s.height)];
}

- (float)width
{
	return width;
}

- (void)setWidth:(float)w
{
	[self setNeedsDisplay];
	width = MIN(MAX(w, 40), 160);
	[self setNeedsDisplay];
	needsRefresh = YES;
}

- (NSRect)frame
{
	return NSMakeRect(origin.x, origin.y - kTitleBarHeight, width, height + kTitleBarHeight);
}

- (void)setIcon:(NSImage *)i
{
	[icon release];
	icon = [i retain];
}

- (BOOL)selected
{
	return selected;
}

- (void)setSelected:(BOOL)s
{
	if (s != selected)
	{
		needsRefresh = YES;
		[self setNeedsDisplay];
	}
	selected = s;
}

- (void)clearSelection
{
	[self setSelected:NO];
}

- (NSArray *)multiports
{
	NSMutableArray *a = [NSMutableArray array];
	NSEnumerator *e = [[Jack multiports] objectEnumerator];
	JPMultiport *m;

	while (m = [e nextObject])
		if ([m client] == self)
			[a addObject:m];
	
	return a;
}

- (NSArray *)multiportsForType:(int)t
{
	NSMutableArray *a = [NSMutableArray array];
	NSEnumerator *e = [[Jack multiports] objectEnumerator];
	JPMultiport *m;

	while (m = [e nextObject])
		if ([m client] == self && [m type] == t)
			[a addObject:m];
	
	return a;
}

- (NSArray *)multiportsForPort:(NSString *)p
{
	if (p == nil) return nil;
	
	NSMutableArray *a = [NSMutableArray array];
	NSEnumerator *e = [[Jack multiports] objectEnumerator];
	JPMultiport *m;

	while (m = [e nextObject])
		if ([m client] == self)
			if ([m referencesPort:p])
				[a addObject:m];
	
	return a;
}

- (NSArray *)connections
{
	NSMutableArray *a = [NSMutableArray array];
	NSArray *multiports = [self multiports];
	NSEnumerator *e = [[Jack connections] objectEnumerator];
	JPConnection *c;
	
	while (c = [e nextObject])
	{
		if ([multiports containsObject:[c inputPort]] || [multiports containsObject:[c outputPort]])
			[a addObject:c];
	}
	
	return a;
}

@end
