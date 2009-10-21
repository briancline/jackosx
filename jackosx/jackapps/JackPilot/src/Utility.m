/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import "Utility.h"

id newplist;

@implementation Utility
+ (int) error:(int) err {
    switch(err) {
        case 'audi':
        {
			NSString *mess1 = NSLocalizedString(@"Error:",nil);
			NSString *mess2 = NSLocalizedString(@"Cannot initialize CoreAudio services, check JackPilot preferences or quit. ", nil);
			NSString *mess3 = NSLocalizedString(@"Preferences:",nil);
			NSString *mess4 = NSLocalizedString(@"Quit",nil);
            int a = NSRunCriticalAlertPanel(mess1,mess2,mess3,mess4,nil);
            return a;
        }
            
        case 'aud2':
        {
			NSString *mess1 = NSLocalizedString(@"Fatal error:",nil);
			NSString *mess2 = NSLocalizedString(@"Cannot initialize CoreAudio services, JackPilot will quit. ", nil);
			NSString *mess4 = NSLocalizedString(@"Quit",nil);
            int a = NSRunCriticalAlertPanel(mess1,mess2,mess4,nil,nil);
            return a;
        }
            
        case 'aud3':
        {
			NSString *mess1 = NSLocalizedString(@"Fatal error:",nil);
			NSString *mess2 = NSLocalizedString(@"Cannot initialize CoreAudio services, check preferences. ", nil);
			NSString *mess4 = NSLocalizedString(@"Ok",nil);
            int a = NSRunCriticalAlertPanel(mess1,mess2,mess4,nil,nil);
            return a;
        }
            
        case 'aud4':
        {
			NSString *mess1 = NSLocalizedString(@"Warning:",nil);
			NSString *mess2 = NSLocalizedString(@"Cannot initialize CoreAudio services, check preferences. ", nil);
			NSString *mess4 = NSLocalizedString(@"Ok",nil);
            int a = NSRunAlertPanel(mess1,mess2,mess4,nil,nil);
            return a;
        }
            
        
        case 'audd':
        {
			NSString *mess1 = NSLocalizedString(@"Warning:",nil);
			NSString *mess2 = NSLocalizedString(@"Cannot find last audio device, please check preferences. ", nil);
			NSString *mess3 = NSLocalizedString(@"Preferences:",nil);
			NSString *mess4 = NSLocalizedString(@"Quit",nil);
			NSString *mess5 = NSLocalizedString(@"Use default",nil);
            int a = NSRunAlertPanel(mess1,mess2,mess3,mess4,mess5);
            return a;
        }
        
        case 'jasN':
        {
			NSString *mess1 = NSLocalizedString(@"Warning:",nil);
			NSString *mess2 = NSLocalizedString(@"Jack Audio Server CoreAudio driver is not installed or is corrupted. ", nil);
			NSString *mess4 = NSLocalizedString(@"Ok",nil);
            int a = NSRunAlertPanel(mess1,mess2,mess4,nil,nil);
            return a;
        }
    }
    return -55;
}

+ (BOOL) initPreference {
    id plist;
    NSString *homeDir = NSHomeDirectory();
    plist = [NSDictionary dictionaryWithContentsOfFile:[homeDir stringByAppendingString:@"/Library/Preferences/JackPilot.plist"]];
    newplist = [[NSMutableDictionary dictionaryWithCapacity:2] retain];
    [newplist addEntriesFromDictionary:plist];
    return (plist) ? YES: NO;
}

+ (BOOL) savePref:(id) array prefType:(int) type {
    
    switch(type) {
            
        case 'audi':
        {
            [newplist setObject:array forKey:@"audi"];
        }
        break;
            
        case 'jacd':
        {
            [newplist setObject:array forKey:@"jacd"];
        }
        break;
            
		case 'winP':
        {
            [newplist setObject:array forKey:@"winP"];
        }
        break;
            
		case 'PlSL':
        {
            [newplist setObject:array forKey:@"PlSL"];
        }
        break;
            
		case 'PlOp':
        {
            [newplist setObject:array forKey:@"PlOp"];
        }
        break;
            
    };
    
    NSString *homeDir = NSHomeDirectory();
    [newplist writeToFile:[homeDir stringByAppendingString:@"/Library/Preferences/JackPilot.plist"] atomically:NO];
    return YES;
}

+ (id) getPref:(int) type {
    
    id plist;
    NSString *homeDir = NSHomeDirectory();
    plist = [NSDictionary dictionaryWithContentsOfFile:[homeDir stringByAppendingString:@"/Library/Preferences/JackPilot.plist"]];
    [newplist addEntriesFromDictionary:plist];
    
    switch(type) {
            
        case 'audi':
        {
            NSArray *res = [newplist objectForKey:@"audi"];
            return res;
        }
        break;
            
        case 'jacd':
        {
            NSArray *res = [newplist objectForKey:@"jacd"];
            return res;
        }
        break;
            
		case 'winP':
        {
            NSArray *res = [newplist objectForKey:@"winP"];
            return res;
        }
        break;
            
		case 'PlSL':
        {
            NSString *res = [newplist objectForKey:@"PlSL"];
            return res;
        }
        break;
            
		case 'PlOp':
        {
            NSArray *res = [newplist objectForKey:@"PlOp"];
            return res;
        }
        break;
            
    };
    
    return nil;
}

@end
