/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "DataSourceSend.h"
#include <stdio.h>

@interface portsArrData : NSObject {
    NSString *nomePorta;
    char *nomeP;
    int kind;
    int IDporta;
    int isConn;
}

-(void)setName:(const char *)nome;
-(void)setNameB:(NSString *)nome;
-(NSString*)getName;
-(void)getChars:(char*)IN;
-(void)setTipo:(int)tipo;
-(int)getTipo;
-(void)setID:(int)lId;
-(int)getID;
-(void)setNameCh:(NSString*)nome ;
-(const char *)getCPort ;
-(void) setIsConn:(int)arg;
-(int) IsConn;

@end