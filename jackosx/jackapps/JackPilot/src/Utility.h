/*this is under GPL license.						*/
/*check license.txt for more infos					*/
/*Copyright (c) 2004 Johnny Petrantoni, jackosx.com */

#import <Foundation/Foundation.h>

@interface Utility : NSObject {    
}

+ (int) error:(int) err;
+ (BOOL) initPreference;
+ (BOOL) savePref:(id) array prefType:(int) prefType;
+ (id) getPref:(int) type;
@end
