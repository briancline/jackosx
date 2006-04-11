#import <Cocoa/Cocoa.h>

@class JPMultiport;

@interface JPClient : NSObject
{
	NSMutableArray *inputPorts;
	NSMutableArray *outputPorts;
	
	NSString *name;
	NSString *driverName;
	
	NSPoint origin;
	float width, height;
	NSImage *image;
	BOOL needsRefresh;
	
	NSColor *color;
	NSImage *icon, *resize;
	NSImage *underlay, *overlay, *gradient;
	NSShadow *iconShadow;
	NSDictionary *titleAttributes;
	
	BOOL selected;
}

- (id)initWithName:(NSString *)n driverName:(NSString *)d;
+ (id)clientWithName:(NSString *)n driverName:(NSString *)d;
- (JPClient *)splitDriver;

- (void)addPortWithName:(NSString *)p type:(int)t;
- (void)removePortWithName:(NSString *)p type:(int)t;

- (BOOL)mouseDown:(NSEvent *)event;
- (void)drag:(NSEvent *)event;
- (void)resize:(NSEvent *)event;

- (void)draw:(NSRect)updateRect;
- (void)setNeedsDisplay;

- (NSDictionary *)state;
- (BOOL)hasPorts;
- (NSMutableArray *)inputPorts;
- (NSMutableArray *)outputPorts;
- (NSMutableArray *)portsOfType:(int)t;
- (NSString *)portNameAtIndex:(unsigned int)i type:(int)t;
- (NSString *)portFullNameAtIndex:(unsigned int)i type:(int)t;
- (unsigned int)indexOfPort:(NSString *)p type:(int)t;
- (NSString *)name;
- (NSString *)driverName;
- (NSImage *)icon;
- (NSPoint)origin;
- (void)setOrigin:(NSPoint)p;
- (void)offsetOrigin:(NSSize)s;
- (float)width;
- (void)setWidth:(float)w;
- (NSRect)frame;
- (void)setIcon:(NSImage *)i;
- (BOOL)selected;
- (void)setSelected:(BOOL)s;
- (NSArray *)multiports;
- (NSArray *)multiportsForType:(int)t;
- (NSArray *)multiportsForPort:(NSString *)p;
- (NSArray *)connections;

@end
