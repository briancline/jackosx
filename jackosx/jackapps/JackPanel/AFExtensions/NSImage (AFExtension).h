@interface NSImage (AFExtension)

+ (NSImage *)rotate:(NSImage *)image byAngle:(int)degrees;

- (void)sliceScaleToRect:(NSRect)dstRect operation:(NSCompositingOperation)op fraction:(float)delta;
- (NSRect)bounds;

@end
