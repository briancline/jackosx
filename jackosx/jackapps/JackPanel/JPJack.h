#import <Cocoa/Cocoa.h>
#import <jack/jack.h>

@class JPClient;
@class JPMultiport;
@class JPView;
@class JPTipView;

@interface JPJack : NSObject
{
	NSMutableArray *clients;
	NSMutableArray *multiports;
	NSMutableArray *connections;
	
	NSMutableDictionary *collection;
	NSTimer *collectionTimer;
	
	IBOutlet NSWindow *window;
	IBOutlet JPView *view;
	IBOutlet JPTipView *tipView;
	
	JPClient *editingClient;
	NSMutableArray *editingClientInputMultiports;
	NSMutableArray *editingClientOutputMultiports;
	IBOutlet NSPanel *clientEditorPanel;
	IBOutlet NSImageView *clientIconImage;
	IBOutlet NSTextField *clientNameText;
	IBOutlet NSTextField *clientDescriptionText;
	IBOutlet NSTableView *clientInputMultiportTable;
	IBOutlet NSTableView *clientInputPortTable;
	IBOutlet NSButton *clientAddInputMultiportButton;
	IBOutlet NSButton *clientRemoveInputMultiportButton;
	IBOutlet NSTableView *clientOutputMultiportTable;
	IBOutlet NSTableView *clientOutputPortTable;
	IBOutlet NSButton *clientAddOutputMultiportButton;
	IBOutlet NSButton *clientRemoveOutputMultiportButton;
	
	IBOutlet NSPanel *saveSetupPanel;
	IBOutlet NSTextField *saveSetupNameText;
}

- (void)updateLoadSetupMenu;

- (NSMutableArray *)currentJackConnections;

- (void)collectCommand:(NSString *)key object:(id)obj;
- (void)startCollectionTimer:(id)obj;
- (void)performCollection:(NSTimer *)timer;
- (void)addPorts:(NSDictionary *)d;
- (void)removePorts:(NSDictionary *)d;
- (void)updateConnections;

- (void)loadClients:(NSArray *)a;
- (void)loadConnections:(NSArray *)a;
- (void)connectMultiport:(JPMultiport *)outputMultiport toMultiport:(JPMultiport *)inputMultiport;
- (void)removeConnections:(NSArray *)c;
- (void)removeClients:(NSArray *)c;
- (void)bringClientToFront:(JPClient *)c;
- (void)clearAllSelections;
- (void)clearClientSelections;
- (void)clearConnectionSelections;

- (IBAction)loadSetup:(id)sender;
- (IBAction)saveSetup:(id)sender;
- (IBAction)editSetups:(id)sender;
- (IBAction)openJackControl:(id)sender;
- (IBAction)openAudioMIDISetup:(id)sender;

- (NSArray *)clients;
- (NSArray *)selectedClients;
- (JPClient *)clientWithName:(NSString *)n;
- (JPClient *)driverClientForType:(int)t;
- (NSMutableArray *)multiports;
- (JPMultiport *)multiportMatchingMultiport:(JPMultiport *)p;
- (NSMutableArray *)connections;
- (NSArray *)selectedConnections;
- (JPView *)view;
- (JPTipView *)tipView;
- (NSWindow *)window;

@end
