#import "Jack.h"

@implementation JPConnection

- (id)init
{
	self = [super init];
	
	inputPort = outputPort = nil;
	path = nil;
	[NSBezierPath setDefaultLineWidth:2.5];
	[NSBezierPath setDefaultLineCapStyle:NSRoundLineCapStyle];
	selected = NO;
	
	return self;
}

- (id)initWithInputPort:(JPMultiport *)i outputPort:(JPMultiport *)o
{
	self = [self init];
	
	inputPort = i;
	outputPort = o;
	
	return self;
}

+ (id)connectionWithInputPort:(JPMultiport *)i outputPort:(JPMultiport *)o
{
	return [[[JPConnection alloc] initWithInputPort:i outputPort:o] autorelease];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"connection input: %@ - output: %@", inputPort, outputPort];
}

- (void)dealloc
{
	[self setNeedsDisplay];
	[path release];
	[super dealloc];
}

//
#pragma mark DRAWING
//

- (void)draw:(NSRect)updateRect
{
	NSBezierPath *drawPath = [NSBezierPath bezierPath];
	NSRect inputFrame, outputFrame;
	NSPoint a, b;
	NSPoint c1, c2;
	
	if (outputPort != nil)
	{
		outputFrame = [outputPort frame];
		a = NSMakePoint(NSMaxX(outputFrame), NSMidY(outputFrame));
	}
	else
		a = [[Jack view] convertPoint:[[NSApp currentEvent] locationInWindow] fromView:nil];
	
	if (inputPort != nil)
	{
		inputFrame = [inputPort frame];
		b = NSMakePoint(NSMinX(inputFrame), NSMidY(inputFrame));
	}
	else
		b = [[Jack view] convertPoint:[[NSApp currentEvent] locationInWindow] fromView:nil];
	
	if ([inputPort client] == [outputPort client])
	{
		c1.x = a.x + 40; c1.y = a.y + 80;
		c2.x = b.x - 40; c2.y = b.y + 80;
	}
	else
	{
		c1.x = a.x + MAX(30, (b.x - a.x) * 0.75); c1.y = a.y;
		c2.x = b.x - MAX(30, (b.x - a.x) * 0.75); c2.y = b.y;
	}
	
	if (selected)
		[[NSColor keyboardFocusIndicatorColor] set];
	else
		[[NSColor blackColor] set];
	[drawPath moveToPoint:a];
	[drawPath curveToPoint:b controlPoint1:c1 controlPoint2:c2];
	[drawPath stroke];
	
	[path release];
	path = [[drawPath bezierPathByFlatteningPath] retain];
}

- (void)setNeedsDisplay
{
	[[Jack view] setNeedsDisplayInRect:[self frame]];
}

//
#pragma mark MOUSING
//

- (BOOL)containsPoint:(NSPoint)p
{
	return [self intersectsRect:NSMakeRect(p.x - 2, p.y - 2, 4, 4)];
}

- (BOOL)intersectsRect:(NSRect)r
{
	NSRect lr = NSZeroRect;
	NSPoint p, q;
	int i, count = [path elementCount];
	BOOL found = NO;
	
	[path elementAtIndex:0 associatedPoints:&p];
	for(i=1;i<count && !found;i++)
	{
		[path elementAtIndex:i associatedPoints:&q];
		lr.origin.x = MIN(p.x, q.x);
		lr.origin.y = MIN(p.y, q.y);
		lr.size.width = fabsf(p.x - q.x);
		lr.size.height = fabsf(p.y - q.y);
		p = q;
		found = NSIntersectsRect(lr, r);
	}
	
	return found;
}

//
#pragma mark ACCESS
//

- (NSArray *)state
{
	NSArray *inputPortNames = [inputPort portFullNames];
	NSArray *outputPortNames = [outputPort portFullNames];
	NSMutableArray *s = [NSMutableArray array];
	int i, count = [inputPortNames count];
	
	for(i=0;i<count;i++)
		[s addObject:[NSDictionary dictionaryWithObjectsAndKeys:
						[inputPortNames objectAtIndex:i], @"input",
						[outputPortNames objectAtIndex:i], @"output",
						nil]];
	
	return s;
}

- (JPMultiport *)inputPort
{
	return inputPort;
}

- (void)setInputPort:(JPMultiport *)p
{
	inputPort = p;
}

- (JPMultiport *)outputPort
{
	return outputPort;
}

- (void)setOutputPort:(JPMultiport *)p
{
	outputPort = p;
}

- (NSRect)frame
{
	return ([path isEmpty] ? NSZeroRect : NSInsetRect([path bounds], -2, -2));
}

- (BOOL)selected
{
	return selected;
}

- (void)setSelected:(BOOL)s
{
	if (s != selected)
		[self setNeedsDisplay];
	selected = s;
}

- (void)clearSelection
{
	[self setSelected:NO];
}

@end
