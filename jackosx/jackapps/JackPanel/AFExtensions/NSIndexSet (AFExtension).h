@interface NSIndexSet (AFExtension)

- (BOOL)intersectsIndexes:(NSIndexSet *)indexes;
- (NSIndexSet *)intersectionIndexes:(NSIndexSet *)indexes;

@end
