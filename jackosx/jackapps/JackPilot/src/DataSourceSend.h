/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Foundation/Foundation.h>
#include "jackfun.h"
#import "PortsData.h"

@interface DatiSend : NSObject {
    int nPorte;
    char *nomeCliente;
    char **nomiPorte;
    NSMutableArray *porte;
    NSMutableArray *idPorte;
    NSMutableArray *portIsConnected;
    int tipoCliente;
    int isConnected;
}

-(int)getNPorte;
-(id)getNomePorta:(int)n;
-(id)getNomeCliente;
-(int)getTipoCliente;
-(void)setData:(int)nporte nomiPorte:(NSMutableArray*)nomi nomeCliente:(const char*)nome kind:(int)tipo;
-(int)getIsConn;
-(void)destroy;
@end

@interface tableDataB : NSObject {
    int quanteporte;
    char *porta;
    id textField1,textField2;
    int stat;
    int whatKind;
    NSMutableArray *data;
    int richiesta;
    int conto;
    int nporte;
    int selectedPort;
    int quantiCli;
    int *needsRelo;
    int *porteSelected;
    id selectedItem;
    int *quanteXCli;
    int oldTest;
    int *chi;
    int quantiItem;
    int kindCliItem;
    id theOutline;
    NSMutableArray *itemsToRelease;
}

- (NSMutableArray*)startPoint;

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item;
- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item ;
- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item ;
- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
- (BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item ;

-(void)writeData: (id)sender text:(NSMutableArray*)testo1 text2:(int*)testo2 text3:(int*)portSelected text4:(int*)quantePCli text5:(int*)chiSelected;
-(id)getCHisono: (int)numero;
-(NSMutableArray*)getPorteSelected;
-(id)getCHisono2: (int)numero;
-(void)flush:(id)sender;
-(void)setWhatKind: (int)n;
-(NSMutableArray*)getPortsForClient:(NSString*)nome withData:(NSMutableArray*)data ;
-(void) needsReload ;
-(int)getQuantiItem;
-(int)getTipoString;
-(id)getCurrentItem;
-(void) selezionaPorte;
@end
