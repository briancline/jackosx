/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "PortsData.h"
#include "jackfun.h"

@implementation portsArrData

-(void)setName:(const char *)nome {
    nomePorta = [NSString init];
    nomePorta = [NSString stringWithCString:nome];
}

-(void)setNameCh:(NSString*)nome {
    if (nomeP != NULL) 
		free(nomeP);
    nomeP = (char*)malloc(sizeof(char) * 256);
    [nome getCString:nomeP];
}

-(const char *)getCPort {
    if (nomeP != NULL) 
		return nomeP;
    return NULL;
}

-(void)setNameB:(NSString *)nome {
    nomePorta = nome;
}

-(NSString*)getName {
    return nomePorta;
}

-(void)getChars:(char*)IN {
    char *buf = (char*)alloca(256 * sizeof(char));
    [nomePorta getCString:buf];
    strcpy(IN,buf);
}

-(void)setTipo:(int)tipo {
    kind = tipo;
}
-(int)getTipo {
    return kind;
}

-(void)setID:(int)lId {
    IDporta = lId;
}

-(int)getID {
    return IDporta;
}

-(void) setIsConn:(int)arg {
    isConn = arg;
}

-(int) IsConn {
    return isConn;
}

@end