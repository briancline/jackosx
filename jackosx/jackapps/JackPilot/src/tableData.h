/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */


#import <Foundation/Foundation.h>
#include "jackfun.h"

@interface tableData : NSObject {
    int quanteporte;
	id textField1,textField2;
    int stat;
    int whatKind;
    int *porteSelected;
    NSMutableArray *porte;
    int *quanteSel;
    int *needsRelo;
    id arrayOk;
}

- (id)tableView:(NSTableView *)aTableView
    objectValueForTableColumn:(NSTableColumn *)aTableColumn
    row:(int)rowIndex;
        
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
    
-(void)writeData: (id)sender text:(int*)testo1 text2:(int*)testo2 text3:(int*)needsReload;
-(id)getCHisono: (int)numero;
-(void)flush:(id)sender;
-(void)setWhatKind: (int)n;
-(void)reorderTheArray;
@end
