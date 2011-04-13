/* JackMenu */
/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Cocoa/Cocoa.h>
#include "jackfun.h"
#include "bequite.h"
#include <CoreAudio/CoreAudio.h>
#import "Utility.h"

#define LOCSTR(s) NSLocalizedString(s,nil)

@interface JackMenu : NSObject
{
    IBOutlet id bufferText; 
    IBOutlet id outputChannels;  
	IBOutlet id inputChannels;
	
  	id verboseBox;
    id hogBox;
    id clockBox;
    id monitorBox;
    id MIDIBox;
    IBOutlet NSPopUpButton* driverBox;
    IBOutlet NSPopUpButton* interfaceInputBox;
    IBOutlet NSPopUpButton* interfaceOutputBox;
    IBOutlet NSTextField* isonBut;
    IBOutlet NSProgressIndicator *cpuLoadBar;
    IBOutlet NSTextField* loadText;
    IBOutlet id samplerateText;
    IBOutlet NSButton* startBut;
    IBOutlet NSWindow* managerWin;
	id jpWinController;
    IBOutlet NSTextField* JALin;
    IBOutlet NSTextField* JALout;
    IBOutlet NSTextField* connectionsNumb;
    IBOutlet NSButton* JALauto;
    IBOutlet NSMenuItem* toggleDock;
    NSTimer* update_timer;
    int jackstat;
    id prefWindow;
    NSString *jasVer,*jackVer,*jpVer,*jpCopyR,*jasCopyR,*jackCopyR;
    IBOutlet NSTextField *jasVerText;
    IBOutlet NSTextField *jackVerText;
    IBOutlet NSTextField *jpVerText;
    IBOutlet NSTextField *jpCopyRText;
    IBOutlet NSTextField *jasCopyRText;
    IBOutlet NSTextField *jackCopyRText;
    NSModalSession modalSex;
    id aboutWin;
    NSString *jpPath;
    char  selectedInputDevice[256];
    char  selectedOutputDevice[256];
    AudioDeviceID selInputDevID;
    AudioDeviceID selOutputDevID;
    id defInput, defOutput, sysDefOut;
	id pluginsMenu;     //NSMenu
	id pluginSubMenu;   //NSMenu
	
	NSMutableArray *plugins_ids;
	id routingBut;
	
}
- (BOOL)writeDevNames;
- (IBAction) reloadPref:(id)sender;
- (BOOL) writeCAPref:(id)sender;
-(IBAction)openJackOsxNet:(id)sender;
-(IBAction)openJohnnyMail:(id)sender;
- (void)error:(NSString*)err;
- (IBAction)startJack:(id)sender;
- (IBAction)toggleJack:(id)sender;
- (IBAction)toggle2Jack:(id)sender;
- (IBAction)openDocsFile:(id)sender;
-(void)cpuMeasure;
-(void)setupTimer;
-(void)stopTimer;
-(void)setPrefItem:(BOOL)state;
-(void)warning:(id)sender;
-(IBAction)jackALstore:(id)sender;
- (int)writeHomePath;
- (IBAction)getJackInfo:(id)sender;
- (IBAction)openPrefWin:(id)sender;
- (IBAction)openAMS:(id)sender;
- (IBAction)openSP:(id)sender;
- (IBAction)closePrefWin:(id)sender;
- (IBAction)openAboutWin:(id)sender;
- (IBAction)closeAboutWin:(id)sender;
- (IBAction)closeJackDeamon:(id)sender;
- (IBAction)closeJackDeamon1:(id)sender;
- (bool)launchJackDeamon:(id)sender;
- (void)addPluginSlot;
- (void)closeFromCallback;
- (void)sendJackStatusToPlugins:(BOOL)isOn;

#ifdef PLUGIN
- (void)testPlugin:(id)sender;
- (void)openPlugin:(id)sender;
- (void)closePlugin:(id)sender;
#endif
@end
