#import "NSSegmentedControl (AFExtension).h"

@implementation NSSegmentedControl (AFExtension)

- (int)tagOfSelectedSegment
{
	return [[self cell] tagForSegment:[self selectedSegment]];
}

@end
