#import "NSMenu (AFExtension).h"

@implementation NSMenu (AFExtension)

- (NSMenuItem *)addItemSortedWithTitle:(NSString *)title action:(SEL)action keyEquivalent:(NSString *)keyEquiv
{
	NSMutableArray *itemTitleArray = [NSMutableArray array];
	NSArray *itemArray = [self itemArray];
	NSEnumerator *itemEnum = [itemArray objectEnumerator];
	id iterator;

	while (iterator = [itemEnum nextObject])
		[itemTitleArray addObject:[iterator title]];
	
	[itemTitleArray addObject:title];
	[itemTitleArray sortUsingSelector:@selector(caseInsensitiveCompare:)];
	
	return (NSMenuItem *)[self insertItemWithTitle:title action:action keyEquivalent:keyEquiv atIndex:[itemTitleArray indexOfObject:title]];
}

- (void)setItemTitlesToSmallSystemFont
{
	NSArray *itemArray = [self itemArray];
	NSDictionary *attributes;
	NSMenuItem *item;
	int i, count = [itemArray count];
	
	attributes = [NSDictionary dictionaryWithObject:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]] forKey:NSFontAttributeName];
	
	for (i=0;i<count;i++)
	{
		item = [itemArray objectAtIndex:i];
		[item setAttributedTitle:[[[NSAttributedString alloc] initWithString:[item title] attributes:attributes] autorelease]];
		[[item submenu] setItemTitlesToSmallSystemFont];
	}
}

- (void)setRepresentedObject:(id)obj
{
	NSArray *itemArray = [self itemArray];
	NSMenuItem *item;
	int i, count = [itemArray count];
	
	for(i=0;i<count;i++)
	{
		item = [itemArray objectAtIndex:i];
		[item setRepresentedObject:obj];
		if ([item submenu]) [[item submenu] setRepresentedObject:obj];
	}
}

@end