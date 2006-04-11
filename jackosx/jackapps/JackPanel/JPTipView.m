#import "JPTipView.h"
#import "Jack.h"

@implementation JPTipView

+ (void)initialize
{
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:9], @"Tip Size",
		nil]];
}

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil)
	{
		text = [[NSMutableString alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[text release];
	[super dealloc];
}

//
#pragma mark DRAW
//

- (void)drawRect:(NSRect)updateRect
{	
	[[NSColor colorWithCalibratedRed:0.988 green:1.0 blue:0.765 alpha:0.95] set];
	NSRectFillUsingOperation(NSIntersectionRect([self bounds], updateRect), NSCompositeSourceOver);
	
	[[NSColor lightGrayColor] set];
	NSFrameRect([self bounds]);
	
	[text drawAtPoint:NSMakePoint(3, 3) withAttributes:[NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:[[NSUserDefaults standardUserDefaults] integerForKey:@"Tip Size"]] forKey:NSFontAttributeName]];
}

//
#pragma mark ACCESS
//

- (void)setText:(NSString *)s
{
	if (s == nil)
		[self setHidden:YES];
	else
	{
		NSSize size;
		
		[text setString:s];
		[[[self window] contentView] setNeedsDisplayInRect:[self frame]];
		size = [text sizeWithAttributes:[NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:[[NSUserDefaults standardUserDefaults] integerForKey:@"Tip Size"]] forKey:NSFontAttributeName]];
		size.width = round(size.width + 6); size.height = round(size.height + 6);
		[self setFrameSize:size];
		[self setHidden:NO];
	}
}

- (void)setPosition:(NSPoint)p
{
	NSView *view = [Jack view];
	NSView *contentView = [[self window] contentView];
	NSRect f;
	
	p = [view convertPoint:p toView:contentView];
	f = NSMakeRect(p.x + 2, p.y + 2, NSWidth([self frame]), NSHeight([self frame]));
	
	if (NSMaxX(f) > NSMaxX([contentView bounds]) - 16)
		f.origin.x = p.x - NSWidth(f) - 2;
	
	if (NSMaxY(f) > NSMaxY([contentView bounds]) - 16)
		f.origin.y = p.y - NSHeight(f) - 2;
	
	[contentView setNeedsDisplayInRect:[self frame]];
	[self setFrameOrigin:f.origin];
	[self setNeedsDisplay:YES];
}

@end
