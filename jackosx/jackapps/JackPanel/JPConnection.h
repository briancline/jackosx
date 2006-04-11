#import <Cocoa/Cocoa.h>

@class JPMultiport;

@interface JPConnection : NSObject
{
	JPMultiport *outputPort;
	JPMultiport *inputPort;
	
	NSBezierPath *path;
	BOOL selected;
}

- (id)initWithInputPort:(JPMultiport *)i outputPort:(JPMultiport *)o;
+ (id)connectionWithInputPort:(JPMultiport *)i outputPort:(JPMultiport *)o;

- (void)draw:(NSRect)updateRect;
- (void)setNeedsDisplay;
- (BOOL)containsPoint:(NSPoint)p;
- (BOOL)intersectsRect:(NSRect)r;

- (NSArray *)state;
- (JPMultiport *)inputPort;
- (void)setInputPort:(JPMultiport *)p;
- (JPMultiport *)outputPort;
- (void)setOutputPort:(JPMultiport *)p;
- (NSRect)frame;
- (BOOL)selected;
- (void)setSelected:(BOOL)s;

@end
