#import "HoverButton.h"

@implementation HoverButton

@synthesize isHovering;

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self)
        [self setup];
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self)
        [self setup];
    return self;
}

- (void)setup
{
    trackingRect = 0;
}

- (void)setFrame:(NSRect)frameRect
{
    [super setFrame:frameRect];
    if (trackingRect)
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
