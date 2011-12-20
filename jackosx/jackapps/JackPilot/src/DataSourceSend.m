/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "DataSourceSend.h"
#define DEBUGGO 0


@implementation DatiSend

-(int)getNPorte 
{
    return nPorte;
}

-(id)getNomePorta:(int)n 
{
    if(n>(nPorte-1)) return @"NIL";
    NSString *name = [NSString init];
    name = [NSString stringWithCString:nomiPorte[n]];
    NSArray *items = [name componentsSeparatedByString:@":"];
    NSString *fres = [NSString init];
    if ([items count] == 3) 
		fres = [items objectAtIndex:2];
    else 
		fres = [items objectAtIndex:1];
    portsArrData *datares = [[portsArrData alloc] retain];
    [datares setNameCh:fres];
    [datares setID:[[idPorte objectAtIndex:n] intValue]];
    [datares setIsConn:[[portIsConnected objectAtIndex:n] intValue]];
    return datares;
}

-(int)getIsConn {
    return isConnected;
}

-(id)getNomeCliente 
{
    if (nomeCliente == NULL) 
		return @"NIL";
	NSString *res = [NSString stringWithCString:nomeCliente];
    NSArray *items = [res componentsSeparatedByString:@":"];
    NSString *fres = [NSString init];
    
    switch(isConnected) {
        case 1:
        {
            id style;
            id testo = [[NSAttributedString alloc] autorelease];
            if ([items count] == 2) { 
				fres = [items objectAtIndex:0];
                style = [NSDictionary dictionaryWithObject:[NSColor redColor] forKey:NSForegroundColorAttributeName]; 
			} else { 
                fres = [items objectAtIndex:1];
                style = [NSMutableDictionary dictionaryWithObject:[NSColor redColor] forKey:NSForegroundColorAttributeName]; 
                [style setObject:[NSFont boldSystemFontOfSize:13.0f] forKey:NSFontAttributeName];  
            }
            [testo initWithString:fres attributes:style];
            return testo;
        }
        default:
        {
            if ([items count] == 3) { 
                id testo = [[NSAttributedString alloc] autorelease];
                NSMutableDictionary *style;
                fres = [items objectAtIndex:1]; 
                style = [NSMutableDictionary dictionaryWithObject:[NSFont boldSystemFontOfSize:13.0f] forKey:NSFontAttributeName];
                [testo initWithString:fres attributes:style];
                 return testo; 
            } else 
				fres = [items objectAtIndex:0];
            return fres;
        }
    }
}

-(int)getTipoCliente {
    return tipoCliente;
}

-(void)setData:(int)nporte nomiPorte:(NSMutableArray*)nomi nomeCliente:(const char*)nome kind:(int)tipo {
    nPorte = nporte;
    
    porte = [NSMutableArray arrayWithArray:nomi];
    nomiPorte = (char**)malloc(sizeof(char*)*nPorte);
    idPorte = [[NSMutableArray array] retain];
    portIsConnected = [[NSMutableArray array] retain];
    int i;
    for (i = 0; i < nPorte; i++) {
        nomiPorte[i] = (char*)malloc(sizeof(char) * 256);
        portsArrData *porta;
        porta = [porte objectAtIndex:i];
        NSString *portaName = [porta getName];
        [portaName getCString:nomiPorte[i]];
        NSNumber *idporta = [NSNumber numberWithInt:[porta getID]];
        [idPorte addObject:idporta];
        NSNumber *isconn = [NSNumber numberWithInt:[porta IsConn]];
        if ([porta IsConn] == 1) 
			isConnected = 1;
        [portIsConnected addObject:isconn];
    }
   
    nomeCliente = (char*)malloc(sizeof(char) * 256);
    strcpy(nomeCliente,nome);
    tipoCliente = tipo;
}

-(void)destroy {
    int i;
    for(i = 0; i < nPorte;i++) {
        free(nomiPorte[i]);
    }
    free(nomiPorte);
    free(nomeCliente);
}

@end

@implementation tableDataB

- (NSMutableArray*)startPoint {
    return data;
}

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if(item==nil) return [ [self startPoint] count ];
    if([item isKindOfClass:[portsArrData class]]) return 0;
    return [item getNPorte];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    if (item == nil) { 
		richiesta = 22; 
		return YES; 
	}
    if ([item isKindOfClass:[portsArrData class]]) { richiesta=0; return NO; }
    if ([item isKindOfClass:[DatiSend class]]) return YES;
    if ([item getNPorte]!=0) { richiesta=22; return YES; }
    else { richiesta=22; return NO; }
    return NO;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item {
#ifdef DEBUGGO
    //JPLog("ASK3\n");
    //JPLog("Index: %d\n",index);
#endif
    if (item == nil) return [[self startPoint] objectAtIndex:index];
    if ([item isKindOfClass:[DatiSend class]]) return [item getNomePorta:index];
    if ([item isKindOfClass:[NSString class]]) return item;
    return nil;
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
    theOutline = outlineView;
    *chi = 22;
    if(item==nil) return @"NIL";
#ifdef DEBUGGO
    //JPLog("RICHIESTA : %d\n",richiesta);
#endif
    if ([item isKindOfClass:[portsArrData class]]) {
        const char *buf;
        buf = [item getCPort];
        if (buf != NULL) {
           NSString *res = [NSString stringWithCString:buf];
           selectedPort = [item getID];
           porteSelected[0] = [item getID];
           *quanteXCli = 1;
#ifdef DEBUGGO
           //JPLog("Selezionata: %d\n",selectedPort);
#endif		
            if ([item IsConn] == 1) {
                id testo = [[NSAttributedString alloc] autorelease];
                NSDictionary *style = [NSDictionary dictionaryWithObject:[NSColor redColor] forKey:NSForegroundColorAttributeName];
                [testo initWithString:res attributes:style];
                kindCliItem = 666;
                [self needsReload];
                [itemsToRelease addObject:item];
                return testo;
            }
            kindCliItem = 333;
            [self needsReload];
            [itemsToRelease addObject:item];
            return res; 
        }
        return @"NIL";
    }
    if ([item isKindOfClass:[DatiSend class]]) { 
        //JPLog("CLIENTE SELEZIONATO");
        selectedPort = -10;
        selectedItem = item;
        [self selezionaPorte];
        [self needsReload];
        id result = [item getNomeCliente];
        if ([item getIsConn] != 1) 
            kindCliItem = 333;
        else 
            kindCliItem = 666;
        [itemsToRelease addObject:item];
        return result;
    }
    if (richiesta == 22) 
        return [item getNomeCliente];
    return @"NIL";
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item {
#ifdef DEBUGGO
    //JPLog("ASK4\n");
#endif
    return NO;
}

-(void) selezionaPorte
{
    id theItem = [theOutline itemAtRow:[theOutline selectedRow]];
    if([theItem isKindOfClass:[DatiSend class]]) {
    int quante = [theItem getNPorte];
    int i;
    for(i=0;i<quante;i++) {
        portsArrData *data2;
        data2 = [theItem getNomePorta:i];
        porteSelected[i] = [data2 getID];
#ifdef DEBUGGO
        //char *tempBuf;
        //tempBuf = (char*)alloca(256*sizeof(char*));
        //tempBuf = [data2 getCPort];
        //JPLog("Seleziono porte: %s\n",tempBuf);
#endif
    }
    *quanteXCli = quante;
    *needsRelo=2;
    }
    if([theItem isKindOfClass:[portsArrData class]]) {
        porteSelected[0] = [theItem getID];
#ifdef DEBUGGO
        //char *tempBuf;
        //tempBuf = (char*)alloca(256*sizeof(char*));
        //tempBuf = [theItem getCPort];
        //JPLog("Seleziono porte: %s\n",tempBuf);
#endif
        *quanteXCli = 1;
        *needsRelo=2;
    }
}

-(void)writeData: (id)sender text:(NSMutableArray*)testo1 text2:(int*)testo2 text3:(int*)portSelected text4:(int*)quantePCli text5:(int*)chiSelected
{	
	chi = chiSelected;
	quanteXCli = quantePCli;
	needsRelo = testo2;
	porteSelected = portSelected;
	int n;
	stat = 1;
	n = numeroPorte();
	quanteporte = n;
	data = [[NSMutableArray array] retain];
	itemsToRelease = [[NSMutableArray array] retain];
	quantiItem = 0;
	
	NSMutableArray *names_to_verify = [NSMutableArray array];
	quantiCli = quantiClienti();
	
	int i;
	for (i = 0; i < quantiCli; i++) {
		NSEnumerator *enumerator = [names_to_verify objectEnumerator];
		id anObject;
		BOOL bypass = NO;

		DatiSend *dataToSend = [DatiSend alloc];
		char *nomeCli;
		nomeCli = (char*)alloca(256 * sizeof(char));
		nameOfClient(i,nomeCli);
		int tipoPorta = getTipoByName(nomeCli);
		NSString *testclient = [NSString init];
		testclient = [NSString stringWithCString:nomeCli];
		
		NSArray *split1 = [testclient componentsSeparatedByString:@":"];
		NSString *pre_name = [split1 objectAtIndex:0];
		
		while (anObject = [enumerator nextObject]) {
			NSArray *split0 = [anObject componentsSeparatedByString:@":"];
			if([[split0 objectAtIndex:0] isEqualToString:pre_name]) { JPLog("I've found an old client name, bypassing.\n"); bypass = YES; }
		}
		
		if (bypass) 
            continue;
		
		NSMutableArray *nomi = [self getPortsForClient:testclient withData:testo1];
		if (nomi != nil) { 
			[dataToSend setData:[nomi count] nomiPorte:nomi nomeCliente:nomeCli kind:tipoPorta];
			[data addObject:dataToSend];
			quantiItem++;
		}
		[names_to_verify addObject:testclient];
	}
}

-(void)flush: (id)sender
{
    int i;
    for(i = 0; i < [data count]; i++) {
        [[data objectAtIndex:i] destroy];
    }
    [data removeAllObjects];
    [data release];
    [itemsToRelease removeAllObjects];
    [itemsToRelease release];
}

-(id)getCHisono: (int)numero
{
    if (selectedPort != -10) {
        unsigned long tipo;
        porta = (char*)alloca(256 * sizeof(char));
        id item = [theOutline itemAtRow:[theOutline selectedRow]];
        if ([item isKindOfClass:[portsArrData class]]) {
        	portaPerNumero([item getID], porta, &tipo);
            JPLog("FROM: %s\n",porta);
            NSString *stringa;
            stringa = [NSString stringWithCString:porta];
            return stringa;
        }
    }
    return nil;
}

-(NSMutableArray*)getPorteSelected 
{
    NSMutableArray *result = [[NSMutableArray array] retain];
    id theItem = [theOutline itemAtRow:[theOutline selectedRow]];
    if(![theItem isKindOfClass:[portsArrData class]]) {
        int quante = [theItem getNPorte];
        int i;
        for (i = 0; i < quante; i++) {
            portsArrData *data2 = [theItem getNomePorta:i];
            unsigned long tipo;
            porta = (char*)alloca(256 * sizeof(char));
			portaPerNumero([data2 getID], porta, &tipo);
            NSString *res = [NSString stringWithCString:porta];
            [result addObject:res];
        }
        return result;
    } else {
        unsigned long tipo;
        porta = (char*)alloca(256*sizeof(char));
		portaPerNumero([theItem getID], porta, &tipo);
		NSString *res = [NSString stringWithCString:porta];
        [result addObject:res];
    }
    return result;
}

-(id)getCHisono2: (int)numero
{
    unsigned long tipo;
    porta = (char*)alloca(256 * sizeof(char));
 	portaPerNumero(numero, porta, &tipo);
    JPLog("FROM (saving): %s\n", porta);
    NSString *stringa;
    stringa = [NSString stringWithCString:porta];
    return stringa;
}

-(void)setWhatKind: (int)n
{
    whatKind = n;
}

-(NSMutableArray*)getPortsForClient:(NSString*)nome withData:(NSMutableArray*)data2 {
    NSMutableArray *res = [[NSMutableArray array] retain];
    int i;
    int quantep = [data2 count];
    int written = 0;
    
    NSArray *listaNC = [nome componentsSeparatedByString:@":"];
    NSString *cliente = [listaNC objectAtIndex:0];
    
    for (i = 0;i < quantep; i++) {
        portsArrData *portData = [data2 objectAtIndex:i];
        NSString *name = [portData getName];
        NSArray *lista = [name componentsSeparatedByString:@":"];
        if([[lista objectAtIndex:0] isEqualToString:cliente]) {
			if([portData getTipo] & JackPortIsOutput) {
                portsArrData *sendData = [portsArrData alloc];
                [sendData setNameB:name];
                [sendData setID:[portData getID]];
                [sendData setIsConn:[portData IsConn]];
                [res addObject:sendData];
                written++;
            }
        }
    }
    if (written == 0) 
        return nil;
    return res;
}

-(void) needsReload {
    int n = numeroPorte();
    if (quanteporte != n) { 
		JPLog("Needs Reload, nport changed.\n"); 
		*needsRelo = 22; 
	}
}

-(int)getQuantiItem {
    return selectedPort;
}

-(id)getCurrentItem {
    return selectedItem;
}

-(int)getTipoString {
    id item = [theOutline itemAtRow:[theOutline selectedRow]];
    if ([item isKindOfClass:[portsArrData class]]) { 
        if ([item IsConn] == 1) 
            return 666; 
    }
    else return kindCliItem;
    return 333;
}

@end
