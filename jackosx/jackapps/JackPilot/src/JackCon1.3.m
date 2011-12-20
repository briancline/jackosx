/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "JackCon1.3.h"
#include "jackfun.h"

JackConnections* static_conn;

static void JackPortRegistration(jack_port_id_t port, int a, void *arg) 
{
	JPLog("JackPortRegistration\n");
	JackConnections *c = (JackConnections*)arg;
	[c reload:nil];
}

static void JackPortConnection(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
	JPLog("JackPortConnection\n");
	JackConnections *c = (JackConnections*)arg;
    [c askreload];
}

static int JackGraphOrder(void *arg) 
{
	JPLog("JackGraphOrder\n");
	JackConnections *c = (JackConnections*)arg;
    [c askreload];
	return 0;
}

@implementation JackConnections

int nConnections = 0;

+(JackConnections*)getSelf {
	return static_conn;
}

- (void)JackCallBacks 
{
	JPLog("Setting Jack Graph CallBacks.\n");
 	int res = 0;
	//res = jack_set_port_registration_callback(getClient(), JackPortRegistration, self);
	if (res != 0) 
		JPLog("Cannot: jack_set_port_registration_callback.\n");
	jack_set_graph_order_callback(getClient(), JackGraphOrder, self);
	if (res != 0) 
		JPLog("Cannot: jack_set_graph_order_callback.\n");
    /*
    res = jack_set_port_connect_callback(getClient(), JackPortConnection, NULL);
	if (res != 0) 
		JPLog("Cannot: jack_set_port_connect_callback.\n");
    */
    jack_activate(getClient());
}

-(void)awakeFromNib {
	static_conn = self;
}

- (IBAction)orderFront:(id)sender
{
    if (getStatus() == 1) { 
          
        [self fillPortsArray];
        
        if (datiTab != nil) 
			[datiTab release];
        datiTab = [tableData alloc];
        [datiTab setWhatKind:0];
        [datiTab writeData:sender text:&quantePConnCli text2:&portSelected[0] text3:&needsReload];
        [tabellaPorte setDataSource:datiTab];
        //[tabellaPorte setDrawsGrid:NO];
        [tabellaPorte setGridStyleMask:NSTableViewSolidHorizontalGridLineMask];
        [tabellaPorte setDoubleAction:@selector(removCon:)];
        [tabellaPorte setTarget:self];
		
        if (datiTab2 != nil) 
			[datiTab2 release];
        datiTab2 = [tableDataB alloc];
        [datiTab2 setWhatKind:1];
        [datiTab2 writeData:sender text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected];
        [tabellaSend setDataSource:datiTab2];
        //[tabellaSend setDrawsGrid:NO];
        [tabellaPorte setGridStyleMask:NSTableViewSolidHorizontalGridLineMask];
        [tabellaSend setDoubleAction:@selector(makeCon:)];
        [tabellaSend setTarget:self];
			
        if (datiTab3 != nil) 
			[datiTab3 release];
        datiTab3 = [tableDataC alloc];
        [datiTab3 setWhatKind:1];
        [datiTab3 writeData:sender text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected];
        [tabellaConnect setDataSource:datiTab3];
        //[tabellaConnect setDrawsGrid:NO];
        [tabellaPorte setGridStyleMask:NSTableViewSolidHorizontalGridLineMask];
		[tabellaConnect setDoubleAction:@selector(makeCon:)];
        [tabellaConnect setTarget:self];
		        
        needsReload = 22;
        [self reloadTimer];
        
        [theWindow setOpaque:NO];
        [theWindow makeKeyAndOrderFront:sender];
        
        [nCon setIntValue:getConnections()/2];
		[self setupTimer];
    }
}

- (IBAction)makeCon:(id)sender
{
    doubleClick = YES;
    NSMutableArray *lista1 = nil;
	NSMutableArray *lista2 = nil;
    oldSelection = [tabellaSend selectedRow];
    oldSel2 = [tabellaConnect selectedRow];
    [self selectFrom:sender];
    [self selectTo:sender];
	
    if (kind1 != 1) {
		[[daText stringValue] getCString:daCh];
    } else {
        lista1 = [datiTab2 getPorteSelected];
    }
	
    if (kind2 != 1) {
		[[aText stringValue] getCString:aCh];
    } else {
        lista2 = [datiTab3 getPorteSelected];
    } 
   
    if (kind1 == 1 && kind2 == 0) {
		int n,i;
		n = [lista1 count];
		
		for(i = 0; i < n; i++) {
			[[lista1 objectAtIndex:i] getCString:daCh];
			[[aText stringValue] getCString:aCh];
			
			int test = connectPorts(&daCh[0], &aCh[0]);
			if (test != 0) 
                test = connectPorts(&aCh[0], &daCh[0]);
			if (test != 0) 
                test = disconnectPorts(&daCh[0], &aCh[0]);
			if (test != 0) 
                test = disconnectPorts(&aCh[0], &daCh[0]);
		}
    }
    
    if (kind1 == 0 && kind2 == 0) {
        [[daText stringValue] getCString:daCh];
        [[aText stringValue] getCString:aCh];
		       
        int test = connectPorts(&daCh[0], &aCh[0]);
        if (test != 0) 
            test = connectPorts(&aCh[0], &daCh[0]);
        if (test != 0) 
            test = disconnectPorts(&daCh[0], &aCh[0]);
        if (test != 0) 
            test = disconnectPorts(&aCh[0], &daCh[0]);
    }
    
    if (kind1 == 1 && kind2 == 1) {
        int n,i;
        n = [lista1 count];
		
        for(i = 0; i < n; i++) {
            [[lista1 objectAtIndex:i] getCString:daCh];
            [[lista2 objectAtIndex:i] getCString:aCh];
            int test = connectPorts(&daCh[0] ,&aCh[0]);
            if (test != 0) 
                test = connectPorts(&aCh[0], &daCh[0]);
            if (test != 0) 
                test = disconnectPorts(&daCh[0], &aCh[0]);
            if (test != 0) 
                test = disconnectPorts(&aCh[0], &daCh[0]);
		}
    }
    
    if (kind1 == 0 && kind2 == 1) {
        int n,i;
        n = [lista2 count];
        for (i = 0; i < n; i++) {
            [[lista2 objectAtIndex:i] getCString:aCh];
            [[daText stringValue] getCString:daCh];
            int test = connectPorts(&daCh[0],&aCh[0]);
            if (test != 0) 
                test = connectPorts(&aCh[0], &daCh[0]);
            if (test != 0) 
                test = disconnectPorts(&daCh[0], &aCh[0]);
            if (test != 0) 
                test = disconnectPorts(&aCh[0], &daCh[0]);
		}
    }
	
    needsReloadColor = 44;
}

- (IBAction)reload:(id)sender
{
    [datiTab2 flush:sender];
    [datiTab3 flush:sender];
    [self fillPortsArray];
    [datiTab2 writeData:sender text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected];
    [datiTab3 writeData:sender text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected];
    [tabellaPorte reloadData];
    [tabellaSend reloadData];
    [tabellaConnect reloadData];
    [nCon setIntValue:getConnections()/2];
}

-(void)removeACon:(id)sender 
{}

-(IBAction) reload3:(int)sender {

    [self fillPortsArray];
    
    if (sender == 22) {
        [datiTab2 flush:nil];
        [datiTab2 writeData:self text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected]; 
        [tabellaSend reloadData]; 
    }
    
    if (sender == 21) { 
        [datiTab3 flush:nil];
        [datiTab3 writeData:self text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected]; 
        [tabellaConnect reloadData]; 
    }
    
    if (sender == 50) {
        [datiTab2 flush:nil];
        [datiTab2 writeData:self text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected]; 
        [tabellaSend reloadData];
        
        [datiTab3 flush:nil];
        [datiTab3 writeData:self text:portsArr text2:&needsReload text3:&portSelected[0] text4:&quantePConnCli text5:&chiSelected]; 
        [tabellaConnect reloadData];
    }
    
    [nCon setIntValue:getConnections()/2];
}

- (void)reload2
{
    [tabellaPorte reloadData];
    [nCon setIntValue:getConnections()/2];
}

- (void)askreload
{
    needsReload = 22;
}

- (IBAction)removCon:(id)sender
{
    NSEnumerator *rows = [tabellaPorte selectedRowEnumerator];
    id object;
    
    if (chiSelected == 21) {
        [self selectTo:sender];
        
        if (kind2 == 1) {
            NSMutableArray *lista1;
            lista1 = [datiTab3 getPorteSelected];
            int quante = [lista1 count];
            NSArray *listaPorte = [rows allObjects];
            int i,ia;
            for (i = 0; i < quante; i++) {
                for(ia = 0; ia < [listaPorte count]; ia++) {
                    NSString *chi;
                    chi = [datiTab getCHisono:[[listaPorte objectAtIndex:ia] intValue]];
                    [chi getCString:daCh];
                    
                    NSString *chi2;
                    chi2 = [lista1 objectAtIndex:i];
                    
                    char *buf;
                    buf = (char*)alloca(256 * sizeof(char));
                    [chi2 getCString:buf];
                    
                    int test;
                    test = disconnectPorts(buf,daCh);
                    if (test != 0) test = disconnectPorts(daCh,buf);
                }
            }
        }
        
        if (kind2 == 0) {
            char *buf;
            buf = (char*)alloca(256 * sizeof(char));
            [[aText stringValue] getCString:buf];
            
            while (object = [rows nextObject]) {
                NSString *chi;
                chi = [datiTab getCHisono:[object intValue]];
                [chi getCString:daCh];
                int test;
                test = disconnectPorts(buf,daCh);
                if (test != 0) 
                    test = disconnectPorts(daCh,buf);
            } 
        }
    }
    
    if (chiSelected == 22) {
        [self selectFrom:sender];
        
        if (kind1 == 1) {
            NSMutableArray *lista1;
            lista1 = [datiTab2 getPorteSelected];
            int quante = [lista1 count];
            NSArray *listaPorte = [rows allObjects];
            int i,ia;
            for (i = 0; i < quante; i++) {
                for (ia = 0; ia < [listaPorte count]; ia++) {
                    NSString *chi;
                    chi = [datiTab getCHisono:[[listaPorte objectAtIndex:ia] intValue]];
                    [chi getCString:daCh];
                    
                    NSString *chi2;
                    chi2 = [lista1 objectAtIndex:i];
                    
                    char *buf;
                    buf = (char*)alloca(256 * sizeof(char));
                    [chi2 getCString:buf];
                    
                    int test;
                    test = disconnectPorts(buf,daCh);
                    if (test != 0) 
                        test = disconnectPorts(daCh,buf);
                }
            }
        }
        
        if (kind1 == 0) {
            char *buf;
            buf = (char*)alloca(256 * sizeof(char));
            [[daText stringValue] getCString:buf];
            while (object = [rows nextObject]) {
                NSString *chi;
                chi = [datiTab getCHisono:[object intValue]];
                [chi getCString:daCh];
                int test;
                test = disconnectPorts(buf,daCh);
                if (test != 0) test = disconnectPorts(daCh,buf);
            } 
        }
    }
    
    needsReloadColor = 44;
}

- (IBAction)selectFrom: (id)sender
{
    int chisono = [tabellaSend selectedRow];
    NSString *chi;
    chi = [datiTab2 getCHisono:chisono];
    tipoStringa1 = [datiTab2 getTipoString];
    if (chi == nil) { kind1=1; return; }
    kind1 = 0;
    [daText setStringValue:chi];
}

- (IBAction)selectTo: (id)sender
{
    int chisono = [tabellaConnect selectedRow];
    NSString *chi;
    chi = [datiTab3 getCHisono:chisono];
    tipoStringa2 = [datiTab3 getTipoString];
    if( chi == nil) { kind2=1; return; }
    kind2 = 0;
    [aText setStringValue:chi];
}

- (IBAction)saveScheme: (id)sender
{
	if ([theWindow isVisible]) {

		NSSavePanel *sp;
		int runResult;
		NSString *filename;
		sp = [NSSavePanel savePanel];

		[sp setRequiredFileType:@"jks"];
		runResult = [sp runModalForDirectory:NSHomeDirectory() file:@""];

		if (runResult == NSOKButton) {
			if(!(filename = [sp filename])) {
				NSBeep();
			}
		} else {
			return;
        }
		
		char namefile[256];
		[filename getCString:namefile];
		
		FILE *schemeFile;
		if ((schemeFile = fopen(&namefile[0], "wt")) == NULL) {
			return;
		} else {
			
			int chisono = numeroPorte();
			int chisono2 = getConnections();
			int i;
			
			char **str = (char**)calloc(128, sizeof(char*));
			int rip;
			for (rip = 0; rip < 128; rip++) {
				str[rip] = (char*)calloc(256, sizeof(char));
			}
		   
			fprintf(schemeFile,"\t%d",(chisono2/2)); 
         
			for (i = 0; i < chisono; i++) {
				char strDA[256],strA[256];
				
				NSString *chi = [datiTab2 getCHisono2:i];
				NSString *chi2  = [datiTab3 getCHisono2:i];
				[chi getCString:strDA];
				[chi2 getCString:strA];
				int test = getTipoByName(&strDA[0]);
                    
				//if (test == 22 || test == 2) {  // Was actually testing Output ports...
                
                if (test & JackPortIsOutput) {
					int res = connessionePerNumero2(i, str, 128);
					if (res != 0) {
						int a;
						for (a = 0; a < res; a++) {
							NSString *daStr = [NSString stringWithCString:strA];
							NSString *aStr = [NSString stringWithCString:str[a]];
							NSArray *lista1 = [daStr componentsSeparatedByString:@" "];
							NSArray *lista2 = [aStr componentsSeparatedByString:@" "];
							if ([lista1 count] == 1 && [lista2 count] == 1) {
								JPLog("JackPilot is saving: %s -> %s\n", strA, str[a]);
								fprintf(schemeFile,"\t%s\t%d\t%s\t%d", strA, -1, str[a], -1);
							} 
							if ([lista1 count] != 1 && [lista2 count] == 1) {
								int cici;
								char *buf1, *buf2;
								buf1 = (char*)calloc(256, sizeof(char));
								buf2 = (char*)calloc(256, sizeof(char));
								
								for(cici=0;cici<[lista1 count];cici++) {
									NSString *pre = [lista1 objectAtIndex:cici];
									[pre getCString:buf1];
									strcat(buf2,buf1);
									if(cici+1!=[lista1 count])strcat(buf2,"%%%");
								}
								strcpy(strA, buf2);
								free(buf1); 
                                free(buf2);
								JPLog("JackPilot is saving: %s -> %s\n", strA, str[a]);
								fprintf(schemeFile,"\t%s\t%d\t%s\t%d", strA, -1, str[a], -1);
							}
							if ([lista1 count] != 1 && [lista2 count] != 1) {
								int cici;
								char *buf1, *buf2;
								buf1 = (char*)calloc(256,sizeof(char));
								buf2 = (char*)calloc(256,sizeof(char));
								
								for(cici = 0; cici < [lista1 count]; cici++) {
									NSString *pre = [lista1 objectAtIndex:cici];
									[pre getCString:buf1];
									strcat(buf2,buf1);
									if(cici+1!=[lista1 count])strcat(buf2,"%%%");
								}
								strcpy(strA,buf2);
								free(buf1); 
                                free(buf2);
								
								char *buf3,*buf4;
								buf3 = (char*)calloc(256, sizeof(char));
								buf4 = (char*)calloc(256, sizeof(char));
								
								for (cici = 0; cici <[lista2 count]; cici++) {
									NSString *pre = [lista2 objectAtIndex:cici];
									[pre getCString:buf3];
									strcat(buf4, buf3);
									if (cici + 1 != [lista2 count])
										strcat(buf4,"%%%");
								}
								strcpy(str[a],buf4);
								free(buf3); 
								free(buf4);
								JPLog("JackPilot is saving: %s -> %s\n", strA, str[a]);
								fprintf(schemeFile,"\t%s\t%d\t%s\t%d", strA, -1, str[a], -1);
							}
							
							if ([lista1 count] == 1 && [lista2 count] != 1) {
								int cici;
								char *buf1,*buf2;
								buf1 = (char*)calloc(256, sizeof(char));
								buf2 = (char*)calloc(256, sizeof(char));
								
								for (cici = 0; cici < [lista2 count]; cici++) {
									NSString *pre = [lista2 objectAtIndex:cici];
									[pre getCString:buf1];
									strcat(buf2, buf1);
									if (cici + 1 != [lista2 count])
										strcat(buf2,"%%%");
								}
								strcpy(str[a], buf2);
								free(buf1); 
								free(buf2);
								JPLog("JackPilot is saving: %s -> %s\n",strA, str[a]);
								fprintf(schemeFile,"\t%s\t%d\t%s\t%d", strA, -1, str[a], -1);
							}
						}
					}
				}
			}
			for (rip = 0; rip < 128; rip++) {
				free(str[rip]);
			}
			free(str);
			int stopInt = 12341;
			fprintf(schemeFile,"\t%d", stopInt);
			fclose(schemeFile);
		}
	} else {
		NSRunAlertPanel(LOCSTR(@"Sorry..."),LOCSTR(@"You must have \"Connections Manager\" window opened, and JACK must be ON."), LOCSTR(@"Ok"), nil, nil);
    }
}

- (IBAction)loadScheme: (id)sender
{
    if([theWindow isVisible]) {

    int result;
    char filename[256];
    NSArray *fileTypes = [NSArray arrayWithObject:@"jks"];
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];

    [oPanel setAllowsMultipleSelection:NO];
    result = [oPanel runModalForDirectory:NSHomeDirectory()
                    file:nil types:fileTypes];
    if (result == NSOKButton) {
        NSArray *filesToOpen = [oPanel filenames];
        NSString *aFile = [filesToOpen objectAtIndex:0];
        [aFile getCString:filename];
    } else return;

    FILE *schemeFile;
    if ((schemeFile = fopen(&filename[0], "rt")) == NULL) {
        return;
	} else {
		
		int chisono = numeroPorte();
		int i;
		
		int quanteConn = getConnections()/2;
		
		if (quanteConn != 0) {
			
			for (i = 0; i < chisono; i++) {
				
				NSString *chi2;
				chi2 = [datiTab2 getCHisono2:i];
				[chi2 getCString:aCh];
				
				int quanteCc = connessionePerNumero2(i, NULL, 0);
				
				char **thebuf;
				thebuf = (char**)alloca(quanteCc * sizeof(char*));
				int aa;
				for (aa = 0; aa < quanteCc; aa++) {
					thebuf[aa] = (char*)alloca(256 * sizeof(char));
				}
				
				connessionePerNumero2(i, thebuf, quanteCc);
				
				for (aa = 0; aa < quanteCc; aa++) {
					int test = disconnectPorts(aCh, thebuf[aa]);
					if (test != 0) 
						test = disconnectPorts(thebuf[aa], aCh);
				}
			}
		}
		
		int howMany;
		fscanf(schemeFile,"\t%d",&howMany);
		JPLog("JackPilot is restoring %d connections \n",howMany);
		
		for (i = 0; i < howMany; i++) {
			
			char *test2;
			test2 = (char*)calloc(256, sizeof(char));
			char *strDA;
			strDA = (char*)calloc(256, sizeof(char));
			char *strA;
			strA = (char*)calloc(256, sizeof(char));
			int nullo;
			
			scan2:
			if ((fscanf(schemeFile, "\t%s\t%d", test2, &nullo)) != 2) {
				strcat(strDA,test2);
				strcat(strDA," ");
				int c;
				do c = getc (schemeFile);
				while (isspace(c));
				ungetc(c, schemeFile);
				goto scan2;
			}
			strcat(strDA,test2);
			scan3:
			if ((fscanf(schemeFile,"\t%s\t%d", test2, &nullo)) != 2) {
				strcat(strA, test2);
				strcat(strA, " ");
				int c;
				do c = getc(schemeFile);
				while (isspace(c));
				ungetc(c, schemeFile);
				goto scan3;
			}
			strcat(strA, test2);
			
			if (nullo == 12341) break;
			if (strcmp(strDA, "12341") == 0) break;
			
			NSString *daStr = [NSString stringWithCString:strDA];
			NSString *aStr = [NSString stringWithCString:strA];
			NSArray *lista1 = [daStr componentsSeparatedByString:@"%%%"];
			NSArray *lista2 = [aStr componentsSeparatedByString:@"%%%"];
			
			char *buf1,*buf2;
			buf1 = (char*)calloc(256, sizeof(char));
			buf2 = (char*)calloc(256, sizeof(char));
			int tick;
			for(tick=0;tick<[lista1 count];tick++) {
				NSString *pre = [lista1 objectAtIndex:tick];
				[pre getCString:buf1];
				strcat(buf2,buf1);
				if(tick+1!=[lista1 count])strcat(buf2," ");
			}
			strcpy(strDA,buf2);
			
			char *buf3,*buf4;
			buf3 = (char*)calloc(256, sizeof(char));
			buf4 = (char*)calloc(256, sizeof(char));
			for (tick = 0; tick < [lista2 count]; tick++) {
				NSString *pre = [lista2 objectAtIndex:tick];
				[pre getCString:buf3];
				strcat(buf4,buf3);
				if (tick + 1 != [lista2 count])
					strcat(buf4," ");
			}
			strcpy(strA,buf4);
			
			char buffer[256];
			sprintf(&buffer[0],"%s and %s\n", strDA, strA);
			NSString *warnStr;
			warnStr = [NSString stringWithCString:buffer];
			
			char checkAudio[256];
			char audioDeviceName[256];
			
			memset(&checkAudio, 0, sizeof(char) * 256);
			memset(&audioDeviceName, 0, sizeof(char) * 256);
			int aa;
			for (aa = 0; aa < strlen(strA); aa++) { 
				checkAudio[aa] = strA[aa];
				if (strA[aa] == ':') {
					if (strcmp(checkAudio,"portaudio:") == 0 || strcmp(checkAudio,"coreaudio:") == 0) {
						char audioDevice[256];
						getCurrentAudioDevice(&audioDevice[0]);
						strcat(&checkAudio[0], audioDevice);
						char port[256];
						memset(&port, 0, sizeof(char) * 256);
						int aaa;
						aa++;
						int count = 0;
						for (aaa = aa; aaa < strlen(strA); aaa++) {
							if (strA[aaa] != ':') {
								audioDeviceName[count] = strA[aaa];
								count++;
							}
							if (strA[aaa] == ':') {
								int aaaa;
								int cc=0;
								for (aaaa = aaa; aaaa < strlen(strA); aaaa++) {
									port[cc] = strA[aaaa];
									cc++;
								}
								break;
							}
						}
						char verify[256];
						strcpy(&verify[0],"portaudio:");
						strcat(&verify[0],audioDeviceName);
						if(strcmp(&verify[0],&checkAudio[0])!=0) {
							strcpy(&verify[0],"coreaudio:");
							strcat(&verify[0],audioDeviceName); 
							if(strcmp(&verify[0],&checkAudio[0])!=0) {
								strcat(&checkAudio[0],&port[0]);
								int check2;
								NSMutableString *message = [NSMutableString stringWithCapacity:4];
								[message appendString:LOCSTR(@"The physical driver used with this setup was ")];
								[message appendString:[NSString stringWithCString:&audioDeviceName[0]]];
								[message appendString:LOCSTR(@", now Jack is using ")];
								[message appendString:[NSString stringWithCString:&audioDevice[0]]];
								[message appendString:LOCSTR(@", do you want to restore connections to the used with driver ")];
								[message appendString:[NSString stringWithCString:&audioDevice[0]]];
								[message appendString:@"?"];
								check2 = NSRunCriticalAlertPanel(LOCSTR(@"Warning:"),message,LOCSTR(@"Yes"),LOCSTR(@"No"),nil);
								if (check2 == 0) 
                                    goto end;
								strcpy(strA,&checkAudio[0]);
							}
						}
						break;
					} else break;
				}
			}
			
			memset(&audioDeviceName,0,sizeof(char)*256);
			memset(&checkAudio,0,sizeof(char)*256);
			for (aa = 0; aa < strlen(strDA); aa++) { 
				checkAudio[aa] = strDA[aa];
				if (strDA[aa] == ':') {
					if (strcmp(checkAudio,"portaudio:") == 0 || strcmp(checkAudio,"coreaudio:") == 0) {
						char audioDevice[256];
						getCurrentAudioDevice(&audioDevice[0]);
						strcat(&checkAudio[0],&audioDevice[0]);
						char port[256];
						memset(&port,0,sizeof(char)*256);
						int aaa;
						aa++;
						int count = 0;
						for (aaa = aa; aaa < strlen(strDA); aaa++) {
							if(strDA[aaa]!= ':') {
								audioDeviceName[count] = strDA[aaa];
								count++;
							}
							if (strDA[aaa] == ':') {
								int aaaa;
								int cc=0;
								for(aaaa=aaa;aaaa<strlen(strDA);aaaa++) {
									port[cc] = strDA[aaaa];
									cc++;
								}
								break;
							}
						}
						char verify[256];
						strcpy(&verify[0], "portaudio:");
						strcat(&verify[0], &audioDeviceName[0]);
						if (strcmp(&verify[0], &checkAudio[0])!=0) {
							strcpy(&verify[0], "coreaudio:");
							strcat(&verify[0], audioDeviceName); 
							if (strcmp(&verify[0], &checkAudio[0])!=0) {
								strcat(&checkAudio[0], &port[0]);
								int check2;
								NSMutableString *message = [NSMutableString stringWithCapacity:4];
								[message appendString:LOCSTR(@"The physical driver used with this setup was ")];
								[message appendString:[NSString stringWithCString:&audioDeviceName[0]]];
								[message appendString:LOCSTR(@", now Jack is using ")];
								[message appendString:[NSString stringWithCString:&audioDevice[0]]];
								[message appendString:LOCSTR(@", do you want to restore connections to the used with driver ")];
								[message appendString:[NSString stringWithCString:&audioDevice[0]]];
								[message appendString:@"?"];
								check2 = NSRunCriticalAlertPanel(LOCSTR(@"Warning:"), message, LOCSTR(@"Yes"),LOCSTR(@"No"),nil);
								if (check2 == 0) 
									goto end;
								strcpy(strDA,&checkAudio[0]);
							}
						}
						break;
					} else break;
				}
			}
			
			int test;
			
			test = connectPorts(strDA,strA);
			if (test != 0) 
				test = connectPorts(strA,strDA);
			
			while (test != 0) {
				int a;
				a = NSRunCriticalAlertPanel(warnStr,LOCSTR(@"CANNOT BE RESTORED. If you need this connection you must open client application and retry"),LOCSTR(@"Retry"),LOCSTR(@"Abort"),LOCSTR(@"Skip"));
				switch(a) {
					case 0:
						free(buf3); free(buf4); free(buf1); free(buf2); free(strA); free(strDA); free(test2);
						goto end2;
					case -1:
						goto end;
					case 1:
						break;
				}
				test = connectPorts(strDA,strA);
				if (test != 0) 
					test = connectPorts(strA,strDA);
				if (test == 0) 
					break;
			}
			
	end:
			free(buf3); 
			free(buf4); 
			free(buf1); 
			free(buf2); 
			free(strA); 
			free(strDA); 
			free(test2);
		}
	end2:

		fclose(schemeFile);
	}
    [self reload:sender];
    
    } else 
		NSRunAlertPanel(LOCSTR(@"Sorry..."), LOCSTR(@"You must have \"Connections Manager\" window opened, and JACK must be ON."), LOCSTR(@"Ok"),nil,nil);
}

- (void)fillPortsArray 
{
    if (portsArr != nil) { 
		[portsArr removeAllObjects]; 
		[portsArr release]; 
	} 
    portsArr = [[NSMutableArray array] retain];
    int nports = numeroPorte();
    JPLog("Number of ports: %d\n", nports);
    int i;
    char nomeBuf[256];
    for (i = 0; i < nports; i++) {
        unsigned long tipo;
        portsArrData* data = [[portsArrData alloc] retain];
        portaPerNumero(i, nomeBuf, &tipo);
        JPLog("Port %d : %s of kind: %d\n", i, nomeBuf, tipo);
        int coci;
        for (coci = 0; coci < quantePConnCli; coci++) {
            int connec = connessionePerNumero2(i, NULL, 0);
            if (connec != 0) { 
				char **name = (char**)malloc(connec * sizeof(char*));
				char bufferT[256];
                unsigned long tipo2;
                portaPerNumero(portSelected[coci], bufferT, &tipo2);
                int ii;
                for (ii = 0; ii < connec; ii++) {
              		name[ii] = (char*)malloc(256 * sizeof(char)); 
                }
                connessionePerNumero2(i, name, connec);
                for (ii = 0; ii < connec; ii++) {
                    if (strcmp(bufferT, name[ii]) == 0) 
						[data setIsConn:1];
                }
        		for (ii = 0; ii < connec; ii++) {
                    free(name[ii]);
                }
				free(name);
            } else 
				[data setIsConn:0];
        }
		
        [data setName:nomeBuf];
        [data setTipo:tipo];
        [data setID:i];
        [portsArr addObject:data];
    }
}

-(void) setupTimer
{
    if (update_timer != nil) 
		[update_timer release];
    //update_timer = [[NSTimer scheduledTimerWithTimeInterval: 1.0 target: self selector: @selector(reloadTimer) userInfo:nil repeats:YES] retain];
    update_timer = [[NSTimer scheduledTimerWithTimeInterval: 0.5 target: self selector: @selector(reloadTimer) userInfo:nil repeats:YES] retain];
    [[NSRunLoop currentRunLoop] addTimer: update_timer forMode: NSDefaultRunLoopMode];
}

-(void) stopTimer
{
    [update_timer invalidate];
    [update_timer release];
    update_timer = nil;
}

-(void) reloadTimer {

    if (needsReload == 22) { 
        JPLog("Needs Reload\n");
        needsReload = 0;
        int i;
        int rigo1 = [tabellaSend selectedRow];
        int rigo2 = [tabellaConnect selectedRow];
        
        NSIndexSet * rigo2_set = [tabellaConnect selectedRowIndexes];
        NSIndexSet * rigo1_set = [tabellaSend selectedRowIndexes];
        
        int nrows1 = [tabellaSend numberOfRows];
        int nrows2 = [tabellaConnect numberOfRows];
        NSMutableArray *lista1 = [NSMutableArray array];
        NSMutableArray *lista2 = [NSMutableArray array];
        
        JPLog("rigo1 %d\n", rigo1);
        JPLog("rigo2 %d\n", rigo2);
        JPLog("nrows1 %d\n", nrows1);
        JPLog("nrows2 %d\n", nrows2);
        
        for (i = 0; i < nrows1; i++) {
            id item = [tabellaSend itemAtRow:i];
            if ([tabellaSend isItemExpanded:item]) [lista1 addObject:[NSNumber numberWithInt:i]];
        }
        for (i = 0; i < nrows2; i++) {
            id item = [tabellaConnect itemAtRow:i];
            if ([tabellaConnect isItemExpanded:item]) [lista2 addObject:[NSNumber numberWithInt:i]];
        }
        [self reload:self];
        for (i = 0; i < [lista1 count]; i++) {
            [tabellaSend expandItem:[tabellaSend itemAtRow:[[lista1 objectAtIndex:i] intValue]] expandChildren:YES];
        }
        for ( i = 0; i < [lista2 count]; i++) {
            [tabellaConnect expandItem:[tabellaConnect itemAtRow:[[lista2 objectAtIndex:i] intValue]] expandChildren:YES];
        }
        
        //[tabellaSend selectRow:rigo1 byExtendingSelection:NO];
        //[tabellaConnect selectRow:rigo2 byExtendingSelection:NO];
        
        [tabellaSend selectRowIndexes:rigo1_set byExtendingSelection:NO];
        [tabellaConnect selectRowIndexes:rigo2_set byExtendingSelection:NO];
         
        return;
    }
    
    if (needsReloadColor == 44) {
        [self reload2];
        
        int rigo1 = [tabellaSend selectedRow];
        int rigo2 = [tabellaConnect selectedRow];
        
        NSIndexSet * rigo2_set = [tabellaConnect selectedRowIndexes];
        NSIndexSet * rigo1_set = [tabellaSend selectedRowIndexes];
        
        int nrows1 = [tabellaSend numberOfRows];
        int nrows2 = [tabellaConnect numberOfRows];
		int i;
        
        NSMutableArray *lista1 = [NSMutableArray array];
        NSMutableArray *lista2 = [NSMutableArray array];
        
        for(i = 0; i < nrows1; i++) {
            id item = [tabellaSend itemAtRow:i];
            if ([tabellaSend isItemExpanded:item]) [lista1 addObject:[NSNumber numberWithInt:i]];
        }
        
        for(i = 0; i < nrows2; i++) {
            id item = [tabellaConnect itemAtRow:i];
            if ([tabellaConnect isItemExpanded:item]) [lista2 addObject:[NSNumber numberWithInt:i]];
        }
        
        if (chiSelected == 22 && !doubleClick) 
            [theWindow makeFirstResponder:tabellaSend];
            
        if (chiSelected == 21 && !doubleClick) 
            [theWindow makeFirstResponder:tabellaConnect];
            
        if (chiSelected == 21 && doubleClick) { 
            [theWindow makeFirstResponder:tabellaSend]; 
            [datiTab2 selezionaPorte]; 
        }    
        
        if (chiSelected == 22 && doubleClick) { 
            [theWindow makeFirstResponder:tabellaConnect]; 
            [datiTab3 selezionaPorte]; 
        }
       
        [self reload3:50];
        
        for(i = 0;i < [lista1 count]; i++) {
            [tabellaSend expandItem:[tabellaSend itemAtRow:[[lista1 objectAtIndex:i] intValue]] expandChildren:YES];
        }
        for(i = 0; i < [lista2 count]; i++) {
            [tabellaConnect expandItem:[tabellaConnect itemAtRow:[[lista2 objectAtIndex:i] intValue]] expandChildren:YES];
        }
        
        //[tabellaSend selectRow:rigo1 byExtendingSelection:NO];
        //[tabellaConnect selectRow:rigo2 byExtendingSelection:NO];
        
        [tabellaSend selectRowIndexes:rigo1_set byExtendingSelection:NO];
        [tabellaConnect selectRowIndexes:rigo2_set byExtendingSelection:NO];
        
        oldChi = chiSelected;
        needsReloadColor = 0;
        doubleClick = NO;
		return;
    }
}

-(IBAction) reloadColor:(id)sender {
    needsReloadColor = 44;
}

@end
