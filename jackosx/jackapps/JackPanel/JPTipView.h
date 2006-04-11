#import <Cocoa/Cocoa.h>

@interface JPTipView : NSView
{	
	NSMutableString *text;
}

- (void)setText:(NSString *)s;
- (void)setPosition:(NSPoint)p;

@end
