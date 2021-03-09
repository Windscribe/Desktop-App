#import "HoverButton.h"


@implementation HoverButton

@synthesize isHovering;

- (void)setFrame:(NSRect)frameRect
{
    [super setFrame:frameRect];
    [self removeTrackingRect:trackingRect];
    trackingRect = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
}

- (void)resetCursorRects
{
    [self addCursorRect:[self bounds] cursor:NSCursor.pointingHandCursor];
}

- (void)mouseEntered:(NSEvent*)theEvent
{
    isHovering = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)theEvent
{
    isHovering = NO;
    [self setNeedsDisplay:YES];
}

@end
