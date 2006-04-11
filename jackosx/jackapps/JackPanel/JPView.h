#import <Cocoa/Cocoa.h>

@class JPJack;
@class JPClient;

@interface JPView : NSView
{
	NSRect selectionArea;
}

- (void)dragSelectionArea:(NSEvent *)event;
- (void)updateFrame;

- (NSPoint)defaultInputDeviceOrigin;
- (NSPoint)defaultOutputDeviceOrigin;
- (NSPoint)newClientOrigin;

@end
