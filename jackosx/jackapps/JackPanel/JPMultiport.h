#import <Cocoa/Cocoa.h>

@class JPClient;

@interface JPMultiport : NSObject
{
	JPClient *client;
	int type;
	NSString *name;
	NSMutableIndexSet *ports;
	
	NSPoint origin;
	NSImage *single, *multiple;

	BOOL selected;
}

- (BOOL)isEqualToMultiport:(JPMultiport *)p;

+ (id)multiport;
+ (id)multiportForClient:(JPClient *)c portType:(int)t;

- (void)drag:(NSEvent *)event;

- (void)draw;

- (JPClient *)client;
- (void)setClient:(JPClient *)c;
- (int)type;
- (void)setType:(int)t;
- (NSString *)name;
- (void)setName:(NSString *)n;
- (unsigned int)portCount;
- (NSArray *)portNames;
- (NSArray *)portFullNames;
- (NSIndexSet *)portIndexes;
- (BOOL)referencesPort:(NSString *)p;
- (BOOL)referencesPortIndex:(unsigned int)i;
- (void)setPortIndex:(unsigned int)i state:(BOOL)s;
- (void)addPortIndexes:(NSIndexSet *)s;
- (void)addPortNames:(NSArray *)a;
- (void)removePort:(NSString *)p;
- (NSPoint)origin;
- (void)setOrigin:(NSPoint)p;
- (NSRect)frame;
- (BOOL)selected;
- (void)setSelected:(BOOL)s;
- (BOOL)hasConnections;
- (NSArray *)connections;

@end
