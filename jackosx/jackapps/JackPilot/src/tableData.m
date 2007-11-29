/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "tableData.h"

@implementation tableData

- (int)numberOfRowsInTableView:(NSTableView *)aTableView 
{	
    JPLog("Getting Connections\n");
    if (porte != nil) 
		[porte release];
    porte = [[NSMutableArray array] retain];
    
    int i,b,n,many;
    n = *quanteSel;
    many = 0;
    for (i = 0; i < n; i++) {
        int nn = connessionePerNumero2(porteSelected[i], NULL, 0);
        char **porta = (char**)alloca(nn * sizeof(char*));
        for (b = 0; b < nn; b++) {
            porta[b] = (char*)alloca(256 * sizeof(char));
        }
        connessionePerNumero2(porteSelected[i], porta, nn);
        for (b = 0; b < nn; b++) {
            NSString *nome = [NSString init];
            nome = [NSString stringWithCString:porta[b]];
            [porte addObject:nome];
        }
        many += nn;
    }
    [self reorderTheArray];
    quanteporte = many;
    return quanteporte;
}

- (id)tableView:(NSTableView *)aTableView
    objectValueForTableColumn:(NSTableColumn *)aTableColumn
    row:(int)rowIndex
{
	if (arrayOk == nil || [arrayOk count] == 0) 
		return nil;
    NSString *res = [arrayOk objectAtIndex:rowIndex];
    NSArray *list = [res componentsSeparatedByString:@":"];
    if ([list count] == 3) {
        NSString *primaParte = [[list objectAtIndex:1] stringByAppendingString:@":"];
        NSString *res2 = [primaParte stringByAppendingString:[list objectAtIndex:2]];
        return res2;
    }
    return res;
 }

-(void)writeData: (id)sender text:(int*)testo1 text2:(int*)testo2 text3:(int*)needsReload
{	
	quanteSel = testo1;
	porteSelected = testo2;
	needsRelo = needsReload;
}

-(void)flush: (id)sender
{}

-(id)getCHisono: (int)numero
{
	return [arrayOk objectAtIndex:numero];
}

-(void)setWhatKind: (int)n
{
    whatKind = n;
}

-(void)reorderTheArray {
    if (arrayOk != nil) 
		[arrayOk release];
    arrayOk = [[NSMutableArray array] retain];
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:[porte count]];
    int i;
    NSMutableArray *keys = [NSMutableArray array];
    for (i = 0; i < [porte count]; i++) {
        id str = [porte objectAtIndex:i];
        NSArray *listaStr = [str componentsSeparatedByString:@":"];
        id obj = [listaStr objectAtIndex:0];
        id check = [dict objectForKey:obj];
        if (check == nil) { 
			[keys addObject:obj]; 
			[dict setObject:str forKey:obj]; 
		}
        if (check != nil) { 
            NSMutableArray *array = [NSMutableArray array];
            if ([check isKindOfClass:[NSString class]]) {
                [array addObject:check];
                [array addObject:str];
            }
            if ([check isKindOfClass:[NSMutableArray class]]) {
                int a;
                for (a = 0; a < [check count]; a++) {
                    [array addObject:[check objectAtIndex:a]];
                }
                [array addObject:str];
            }
            [dict setObject:array forKey:obj];
        }
    }
    
    for(i = 0; i < [keys count]; i++) {
        id ports = [dict objectForKey:[keys objectAtIndex:i]]; 
        int a;
        if ([ports isKindOfClass:[NSMutableArray class]]) {
            for (a = 0; a < [ports count]; a++) {
                id objecto = [ports objectAtIndex:a];
                if ([objecto isKindOfClass:[NSString class]]) 
					[arrayOk addObject:[ports objectAtIndex:a]];
                if ([objecto isKindOfClass:[NSMutableArray class]]) 
					[arrayOk addObjectsFromArray:objecto];
            }
        }
        if ([ports isKindOfClass:[NSString class]]) 
			[arrayOk addObject:ports];
    }
}

@end
