#import "NSImage (AFExtension).h"

@implementation NSImage (AFExtension)

+ (NSImage *)rotate:(NSImage *)image byAngle:(int)degrees
{
	if (degrees == 0) return image;

	NSSize beforeSize = [image size];
	NSSize afterSize = (degrees == 90 || degrees == -90 ? NSMakeSize(beforeSize.height, beforeSize.width) : beforeSize);
	NSImage *newImage = [[[NSImage alloc] initWithSize:afterSize] autorelease];
	NSAffineTransform *trans = [NSAffineTransform transform];

	[newImage lockFocus];
	[trans translateXBy:afterSize.width * 0.5 yBy:afterSize.height * 0.5];
	[trans rotateByDegrees:degrees];
	[trans translateXBy:-beforeSize.width * 0.5 yBy:-beforeSize.height * 0.5];
	[trans concat];

	[image drawAtPoint:NSZeroPoint fromRect:NSMakeRect(0, 0, beforeSize.width, beforeSize.height) operation:NSCompositeCopy fraction:1.0];
	[newImage unlockFocus];

	return newImage;
}

- (void)sliceScaleToRect:(NSRect)dstRect operation:(NSCompositingOperation)op fraction:(float)delta
{
	NSRect srcRects[3][3];
	NSRect dstRects[3][3];
	NSSize sourceSize = [self size];
	int x, y;
	
	for(x=0;x<3;x++)
		for(y=0;y<3;y++)
			srcRects[x][y] = NSMakeRect((sourceSize.width * x) / 3, (sourceSize.height * y) / 3, sourceSize.width / 3, sourceSize.height / 3);
	
	// offset the source rects if the dstRect is smaller than the source
	srcRects[0][0].size.width = MIN(NSWidth(srcRects[0][0]), NSWidth(dstRect) / 2);
	srcRects[0][0].size.height = MIN(NSHeight(srcRects[0][0]), NSHeight(dstRect) / 2);
	srcRects[2][0].size.width = MIN(NSWidth(srcRects[2][0]), NSWidth(dstRect) / 2);
	srcRects[2][0].origin.x = sourceSize.width - NSWidth(srcRects[2][0]);
	srcRects[2][0].size.height = MIN(NSHeight(srcRects[2][0]), NSHeight(dstRect) / 2);

	srcRects[0][1].size.width = MIN(NSWidth(srcRects[0][1]), NSWidth(dstRect) / 2);
	srcRects[2][1].size.width = MIN(NSWidth(srcRects[2][1]), NSWidth(dstRect) / 2);
	srcRects[2][1].origin.x = sourceSize.width - NSWidth(srcRects[2][1]);
	
	srcRects[0][2].size.width = MIN(NSWidth(srcRects[0][2]), NSWidth(dstRect) / 2);
	srcRects[0][2].size.height = MIN(NSHeight(srcRects[0][2]), NSHeight(dstRect) / 2);
	srcRects[0][2].origin.y = sourceSize.height - NSHeight(srcRects[0][2]);
	srcRects[2][2].size.width = MIN(NSWidth(srcRects[2][2]), NSWidth(dstRect) / 2);
	srcRects[2][2].origin.x = sourceSize.width - NSWidth(srcRects[2][2]);
	srcRects[2][2].size.height = MIN(NSHeight(srcRects[2][2]), NSHeight(dstRect) / 2);
	srcRects[2][2].origin.y = sourceSize.height - NSHeight(srcRects[2][2]);
	
	// don't scale the corners
	dstRects[0][0] = srcRects[0][0];
	dstRects[2][0].origin = NSMakePoint(NSWidth(dstRect) - NSWidth(srcRects[2][0]), 0); dstRects[2][0].size = srcRects[2][0].size;
	dstRects[0][2].origin = NSMakePoint(0, NSHeight(dstRect) - NSHeight(srcRects[0][2])); dstRects[0][2].size = srcRects[0][2].size;
	dstRects[2][2].origin = NSMakePoint(NSWidth(dstRect) - NSWidth(srcRects[2][2]), NSHeight(dstRect) - NSHeight(srcRects[2][2])); dstRects[2][2].size = srcRects[2][2].size;
	
	// scale sides in one direction
	dstRects[1][0] = NSMakeRect(NSMaxX(dstRects[0][0]), 0, NSMinX(dstRects[2][0]) - NSMaxX(dstRects[0][0]), NSHeight(srcRects[1][0]));
	dstRects[1][2] = NSMakeRect(NSMaxX(dstRects[0][2]), NSMinY(dstRects[0][2]), NSMinX(dstRects[2][2]) - NSMaxX(dstRects[0][2]), NSHeight(srcRects[1][2]));
	dstRects[0][1] = NSMakeRect(0, NSMaxY(dstRects[0][0]) + 1, NSWidth(srcRects[0][1]), NSMinY(dstRects[0][2]) - NSMaxY(dstRects[0][0]) - 2);
	dstRects[2][1] = NSMakeRect(NSMinX(dstRects[2][2]), NSMaxY(dstRects[2][0]) + 1, NSWidth(srcRects[2][1]), NSMinY(dstRects[2][2]) - NSMaxY(dstRects[2][0]) - 2);

	// scale center
	dstRects[1][1] = NSMakeRect(NSMinX(dstRects[1][0]), NSMaxY(dstRects[1][0]) + 1, NSMinX(dstRects[2][1]) - NSMaxX(dstRects[0][1]), NSMinY(dstRects[1][2]) - NSMaxY(dstRects[1][0]) - 2);
	
	// for each rect in matrix
	for(x=0;x<3;x++)
		for(y=0;y<3;y++)
			[self drawInRect:dstRects[x][y] fromRect:srcRects[x][y] operation:op fraction:delta];
}

- (NSRect)bounds
{
	NSRect r = NSZeroRect;
	
	r.size = [self size];
	return r;
}

@end