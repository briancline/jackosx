#import "JPJack.h"
#import "JPJack_ClientEditor.h"
#import "JPClient.h"
#import "JPMultiport.h"
#import "JPConnection.h"
#import "JPView.h"

#define kPortTypeNone -1
#define kPortTypeOutput 0
#define kPortTypeInput 1

#define kTitleBarHeight 18
#define kPortSize 17
#define kDefaultClientWidth 80

#define Jack (JPJack *)[NSApp delegate]
