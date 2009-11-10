/* JackConnections */
/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Cocoa/Cocoa.h>
#import "tableData.h"
#import "DataSourceSend.h"
#import "DataSourceSend2.h"
#include <stdio.h>

#define LOCSTR(s) NSLocalizedString(s,nil)

@interface JackConnections : NSObject
{
    IBOutlet NSProgressIndicator *cpuLoadBar;
    IBOutlet NSTextField *loadText;
    IBOutlet NSTableView *tabellaPorte;
    IBOutlet NSOutlineView *tabellaConnect;
    IBOutlet NSOutlineView *tabellaSend;
    IBOutlet NSWindow *theWindow;
    IBOutlet NSButton *reloadbut;
    tableData *datiTab;
    tableDataB *datiTab2;
    tableDataC *datiTab3;
    IBOutlet NSTextField *daText,*aText,*nCon;
    NSPanel *loadWarn;
    NSTextField *warnText;
    char daCh[256];
    char aCh[256];
    int nConnections;
    NSMutableArray *portsArr;
    NSTimer *update_timer;
    int needsReload;
    int portSelected[256];
    int oldSelection;
    int oldSel2;
    int kind1,kind2;
    int quantePConnCli;
    int chiSelected;
    int oldChi;
    int needsReloadColor;
    int tipoStringa1,tipoStringa2;
    id itemCorrente;
    int oldRigo1,oldRigo2;
    BOOL doubleClick;
}

- (void)JackCallBacks;
-(IBAction) reloadColor:(id)sender;
-(IBAction) reload3:(int)sender;
-(void)removeACon:(id)sender ;
- (IBAction)makeCon:(id)sender;
- (IBAction)orderFront:(id)sender;
- (IBAction)reload:(id)sender;
- (void) reload2;
- (void) askreload;
- (IBAction)removCon:(id)sender;
- (IBAction)selectFrom:(id)sender;
- (IBAction)selectTo:(id)sender;
- (IBAction)saveScheme: (id)sender;
- (IBAction)loadScheme: (id)sender;
-(void) setupTimer;
-(void)stopTimer;
- (void)fillPortsArray;
-(void) reloadTimer;
+(JackConnections*)getSelf;

@end
