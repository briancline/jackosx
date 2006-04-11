@interface NSMenu (AFExtension)

- (NSMenuItem *)addItemSortedWithTitle:(NSString *)title action:(SEL)action keyEquivalent:(NSString *)keyEquiv;
- (void)setItemTitlesToSmallSystemFont;
- (void)setRepresentedObject:(id)obj;

@end
