#import "Jack.h"

@implementation JPView

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil)
	{
		selectionArea = NSZeroRect;
	}
	return self;
}

//
#pragma mark KEY DOWN
//

- (void)keyDown:(NSEvent *)theEvent
{
	if ([theEvent keyCode] == 51)
	{
		NSArray *selection;
		
		selection = [Jack selectedConnections];
		if ([selection count] > 0)
			[Jack removeConnections:selection];
		else
		{
			selection = [Jack selectedClients];
			if ([selection count] > 0)
				[Jack removeClients:selection];
		}
	}
}

//
#pragma mark MOUSING
//

- (void)mouseDown:(NSEvent *)event
{
	NSEnumerator *e;
    NSPoint clickPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	JPClient *c;
	JPConnection *k, *clickedConnection = nil;
	BOOL clearSelection = (!([event modifierFlags] & NSCommandKeyMask) && !([event modifierFlags] & NSShiftKeyMask));
	BOOL handled;
	
	e = [[Jack connections] objectEnumerator];
	while ((k = [e nextObject]) && clickedConnection == nil)
		if ([k containsPoint:clickPoint]) clickedConnection = k;

	if (clickedConnection != nil)
	{
		[Jack clearClientSelections];
		if (clearSelection)
			[Jack clearConnectionSelections];
		[clickedConnection setSelected:YES];
	}
	else
	{
		e = [[Jack clients] objectEnumerator];
		while (c = [e nextObject])
			if (handled = [c mouseDown:event]) break;
		
		if (!handled)
		{
			if (clearSelection) [Jack clearAllSelections];
			[self dragSelectionArea:event];
		}
	}
}

- (void)dragSelectionArea:(NSEvent *)event
{
    NSWindow *window = [Jack window];
	NSPoint curPoint, lastPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSPoint anchorPoint = lastPoint;
	BOOL extendSelection = ([event modifierFlags] & NSCommandKeyMask) || ([event modifierFlags] & NSShiftKeyMask);
	NSArray *connections = [Jack connections];
	JPConnection *c;
	int i, count = [connections count];
	
	while (1)
	{
		event = [window nextEventMatchingMask:(NSLeftMouseDraggedMask | NSLeftMouseUpMask)];
		curPoint = [self convertPoint:[event locationInWindow] fromView:nil];	
		
		if (!NSEqualPoints(curPoint, lastPoint) && [event type] == NSLeftMouseDragged)
		{
			[self setNeedsDisplayInRect:selectionArea];
			selectionArea.origin.x = MIN(curPoint.x, anchorPoint.x);
			selectionArea.origin.y = MIN(curPoint.y, anchorPoint.y);
			selectionArea.size.width = fabsf(curPoint.x - anchorPoint.x);
			selectionArea.size.height = fabsf(curPoint.y - anchorPoint.y);
			[self setNeedsDisplayInRect:selectionArea];
			
			for(i=0;i<count;i++)
			{
				c = [connections objectAtIndex:i];
				if ([c intersectsRect:selectionArea])
					[c setSelected:YES];
				else
				{
					if (!extendSelection)
						[c setSelected:NO];
				}
			}
			
			lastPoint = curPoint;
		}

		if ([event type] == NSLeftMouseUp)
			break;
	}
	
	[self setNeedsDisplayInRect:selectionArea];
	selectionArea = NSZeroRect;
}

- (void)windowDidResize:(NSNotification *)aNotification
{
	[self updateFrame];
}

- (void)updateFrame
{
	NSEnumerator *e = [[Jack clients] objectEnumerator];
	JPClient *c;
	NSRect f = NSZeroRect;

	while (c = [e nextObject])
		f = NSUnionRect(f, [c frame]);
	
	f.size.width += f.origin.x + kPortSize + 1; f.origin.x = 0;
	f.size.height += f.origin.y; f.origin.y = 0;
	[self setFrame:NSUnionRect(f, [[self superview] bounds])];
}

- (BOOL)isFlipped
{
	return YES;
}

//
#pragma mark DRAWING
//

- (void)drawRect:(NSRect)updateRect
{
	NSArray *clients, *connections, *frontConnections;
	JPClient *frontClient;
	JPConnection *connection;
	int i;
	
	clients = [Jack clients];
	connections = [Jack connections];
	frontClient = [clients objectAtIndex:0];
	frontConnections = [frontClient connections];
	
	// clean
	[[NSColor whiteColor] set];
	NSRectFill(updateRect);
	
	// draw all clients except front client
	for(i=[clients count]-1;i>0;i--)
		[[clients objectAtIndex:i] draw:updateRect];
	
	// draw all connections except the ones attributed to the front client
	for(i=0;i<[connections count];i++)
	{
		connection = [connections objectAtIndex:i];
		if ([frontConnections containsObject:connection]) continue;
		[connection draw:updateRect];
	}

	// draw front client
	[frontClient draw:updateRect];
	
	// draw front client's connections 
	for(i=0;i<[frontConnections count];i++)
		[[frontConnections objectAtIndex:i] draw:updateRect];
	
	// draw live selection area
	if (!NSIsEmptyRect(selectionArea))
	{
		[[[NSColor grayColor] colorWithAlphaComponent:0.5] set];
		NSRectFillUsingOperation(selectionArea, NSCompositeSourceOver);
		
		[[NSColor blackColor] set];
		NSFrameRect(selectionArea);
	}
}

//
#pragma mark ACCESS
//

- (NSPoint)defaultInputDeviceOrigin
{
	return NSMakePoint(16, 16 + kTitleBarHeight);
}

- (NSPoint)defaultOutputDeviceOrigin
{
	return NSMakePoint(NSWidth([self bounds]) - kPortSize - kDefaultClientWidth - 1, 16 + kTitleBarHeight);
}

- (NSPoint)newClientOrigin
{
	NSRect r = NSMakeRect(0, 0, kPortSize + kDefaultClientWidth + kPortSize, kDefaultClientWidth + kTitleBarHeight);
	float rHalfWidth = NSWidth(r) / 2;
	NSArray *clients = [Jack clients];
	int i, count = [clients count];
	BOOL intersects = YES;
	
	r.origin = NSMakePoint(NSMidX([self bounds]) - rHalfWidth, 48 + kTitleBarHeight);
	
	while (intersects)
	{
		intersects = NO;
		for(i=0;i<count && !intersects;i++)
			intersects = NSIntersectsRect(r, [[clients objectAtIndex:i] frame]);
		if (intersects)
			r.origin.y += 16 + kTitleBarHeight + kDefaultClientWidth;
	}
	
	return r.origin;
}

@end
