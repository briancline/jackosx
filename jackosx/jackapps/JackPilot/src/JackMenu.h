/* JackMenu */
/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Cocoa/Cocoa.h>
#include "jackfun.h"
#include "bequite.h"
#include <CoreAudio/CoreAudio.h>
#import "Utility.h"
//#include "sys.h"

#define LOCSTR(s) NSLocalizedString(s,nil)


@interface JackMenu : NSObject
{
    NSTask *jackTask;
    IBOutlet id bufferText;
    IBOutlet id channelsTest;
    id jackPilWin;
    id jackdMode;
    IBOutlet NSPopUpButton *driverBox;
    IBOutlet NSPopUpButton *interfaceBox;
    IBOutlet NSTextField *isonBut;
    IBOutlet NSProgressIndicator *cpuLoadBar;
    IBOutlet NSTextField *loadText;
    IBOutlet id samplerateText;
    IBOutlet NSButton *startBut;
    IBOutlet NSWindow *managerWin;
	id jpWinController;
    IBOutlet NSTextField *JALin;
    IBOutlet NSTextField *JALout;
    IBOutlet NSTextField *connectionsNumb;
    IBOutlet NSButton *JALauto;
    IBOutlet NSMenuItem *toggleDock;
    NSTimer *update_timer;
    int jackstat;
    int interfaccia2;
    id prefWindow;
    NSString *jasVer,*jackVer,*jpVer,*jpCopyR,*jasCopyR,*jackCopyR;
    IBOutlet NSTextField *jasVerText;
    IBOutlet NSTextField *jackVerText;
    IBOutlet NSTextField *jpVerText;
    IBOutlet NSTextField *jpCopyRText;
    IBOutlet NSTextField *jasCopyRText;
    IBOutlet NSTextField *jackCopyRText;
    NSModalSession modalSex;
    NSModalSession modalSex2;
    id aboutWin;
    NSString *jpPath;
    char  selectedDevice[256];
    char  oldSelectedDevice[256];
    AudioDeviceID selDevID;
    BOOL jackdStartMode;
    id defInput,defOutput,sysDefOut;
	id pluginsMenu; //NSMenu
	id pluginSubMenu; //NSMenu
	id inputChannels;
	NSMutableArray *plugins_ids;
	id routingBut;
	
}
- (IBAction) switchJackdMode:(id)sender;
- (BOOL)writeDevNames;
- (IBAction) reloadPref:(id) sender;
- (BOOL) writeCAPref:(id)sender;
-(IBAction)openJackOsxNet:(id)sender;
-(IBAction)openJohnnyMail:(id)sender;
- (void)error:(NSString*)err;
- (IBAction)startJack:(id)sender;
- (IBAction)stopJack:(id)sender;
- (IBAction)toggleJack:(id)sender;
- (IBAction)toggle2Jack:(id)sender;
- (IBAction)openDocsFile:(id)sender;
-(void)cpuMeasure;
-(void)setupTimer;
-(void)stopTimer;
-(void)warning:(id)sender;
-(IBAction)jackALstore:(id)sender;
- (int)writeHomePath;
- (IBAction)startJackTask:(id)sender;
- (IBAction)getJackInfo:(id)sender;
- (IBAction)openPrefWin:(id)sender;
- (IBAction)closePrefWin:(id)sender;
- (IBAction) openAboutWin:(id)sender;
- (IBAction) closeAboutWin:(id)sender;
- (int) launchJackCarbon:(id) sender;
- (IBAction) closeJackDeamon:(id) sender;
- (IBAction) launchJackDeamon:(id) sender;
- (void)addPluginSlot;
- (void) sendJackStatusToPlugins:(BOOL)isOn;

#ifdef PLUGIN
- (void)testPlugin:(id)sender;
- (void) openPlugin:(id)sender;
- (void) closePlugin:(id)sender;
#endif
@end