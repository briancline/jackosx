#import "NSIndexSet (AFExtension).h"

@implementation NSIndexSet (AFExtension)

- (BOOL)intersectsIndexes:(NSIndexSet *)indexes
{
	BOOL found = NO;
	int i;
	
	i = [self firstIndex];
	while(i != NSNotFound && !found)
	{
		found = [indexes containsIndex:i];
		i = [self indexGreaterThanIndex:i];
	}
	
	return found;
}

- (NSIndexSet *)intersectionIndexes:(NSIndexSet *)indexes
{
	NSMutableIndexSet *intersection = [NSMutableIndexSet indexSet];
	int i = [self firstIndex];
	
	while (i != NSNotFound)
	{
		if ([indexes containsIndex:i]) [intersection addIndex:i];
		i = [self indexGreaterThanIndex:i];
	}
	
	return intersection;
}

@end