#import "ButtonsCells.h"
#import "HoverButton.h"

// --- Settings button custom draw -------------------------------------
@implementation SettingsButtonCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView *)controlView
{
    [[NSGraphicsContext currentContext] saveGraphicsState];

    NSColor* fillColor = self.backgroundColor;
    [fillColor setStroke];
    [fillColor setFill];
    NSBezierPath* circlePath = [NSBezierPath bezierPath];
    [circlePath appendBezierPathWithOvalInRect: frame];
    [circlePath stroke];
    [circlePath fill];
    
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)drawImage:(NSImage *)image withFrame:(NSRect)frame inView:(NSView *)controlView
{
    HoverButton* button = (HoverButton*)self.controlView;
    NSRect rc = CGRectMake(7, 7, image.size.width, image.size.height);
    if (self.isHighlighted && self.isEnabled)
        [image drawInRect:rc fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:0.9];
    else if (button.isHovering && self.isEnabled)
        [image drawInRect:rc fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:0.8];
    else
        [image drawInRect:rc fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:0.5];
}
@end


// --- Install button custom draw -------------------------------------
@implementation InstallButtonCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView *)controlView
{
    HoverButton* button = (HoverButton*)self.controlView;

    [[NSGraphicsContext currentContext] saveGraphicsState];

    NSColor* fillColor;
    
    if (self.isHighlighted)
        fillColor = [NSColor colorWithCalibratedRed: 1.0 green: 1.0 blue: 1.0 alpha: 1];
    else if (button.isHovering)
        fillColor = [NSColor colorWithCalibratedRed: 0.95 green: 0.95 blue: 0.95 alpha: 1];
    else
        fillColor = [NSColor colorWithCalibratedRed: 0.33 green: 1 blue: 0.54 alpha: 1];

    [fillColor setStroke];
    [fillColor setFill];
    NSBezierPath* roundedPath = [NSBezierPath bezierPathWithRoundedRect:frame xRadius:16 yRadius:16];
    [roundedPath stroke];
    [roundedPath fill];
    
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView
{
    NSColor *textColor = [NSColor colorWithCalibratedRed: 0.0 green: 0.03 blue: 0.1 alpha: 1];
    NSMutableAttributedString *attributedString = [[NSMutableAttributedString alloc] initWithAttributedString:title];
    [attributedString addAttribute:NSForegroundColorAttributeName value:textColor range:NSMakeRange(0, [title length])];
    [attributedString drawInRect: NSMakeRect(frame.origin.x, frame.origin.y - 2, frame.size.width, frame.size.height)];
    return frame;
}

- (void)drawImage:(NSImage *)image withFrame:(NSRect)frame inView:(NSView *)controlView
{
    NSRect rc = CGRectMake(controlView.bounds.size.width - image.size.width - 14, (controlView.bounds.size.height - image.size.height) / 2, image.size.width, image.size.height);
    [image drawInRect:rc];
}
@end

// --- Escape button custom draw -------------------------------------
@implementation EscButtonCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView *)controlView
{
    //[super drawBezelWithFrame:frame inView:controlView];
    // nothing draw, transparent background
}

- (void)drawImage:(NSImage *)image withFrame:(NSRect)frame inView:(NSView *)controlView
{
    NSRect rc = [controlView bounds];
    [image drawInRect: NSMakeRect((rc.size.width - image.size.width) / 2.0f, 2.0f, image.size.width, image.size.height)];
}

- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView
{
    HoverButton *button = (HoverButton*)self.controlView;
    NSColor* color;
    if (button.isHovering)
        color = [NSColor colorWithCalibratedRed: 0.84f green: 0.84f blue: 0.84f alpha: 1.0f];
    else
        color = [NSColor colorWithCalibratedRed: 0.4f green: 0.42f blue: 0.46f alpha: 1.0f];
    
    NSMutableAttributedString *attributedString = [[NSMutableAttributedString alloc] initWithAttributedString:title];
    [attributedString addAttribute:NSForegroundColorAttributeName value:color range:NSMakeRange(0, [title length])];
    
    NSRect rc = [controlView bounds];
    [attributedString drawInRect: NSMakeRect((rc.size.width - attributedString.size.width) / 2.0f,
                                             rc.size.height - attributedString.size.height - 2.0f,
                                             attributedString.size.width, attributedString.size.height)];
    return frame;
}

@end

// --- Eula button custom draw -------------------------------------
@implementation EulaButtonCell

- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView
{
    HoverButton* button = (HoverButton*)self.controlView;
    
    NSColor *color;
    if (button.isHovering)
        color = [NSColor colorWithCalibratedRed: 1.0f green: 1.0f blue: 1.0f alpha: 0.8];
    else
        color = [NSColor colorWithCalibratedRed: 1.0f green: 1.0f blue: 1.0f alpha: 0.5];
    
    NSMutableAttributedString *attributedString = [[NSMutableAttributedString alloc] initWithAttributedString:title];
    [attributedString addAttribute:NSForegroundColorAttributeName value:color range:NSMakeRange(0, [title length])];
    [attributedString drawInRect:frame];
    return frame;
}

@end


// --- Path button custom draw -------------------------------------
@implementation PathButtonCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView *)controlView
{
    //[super drawBezelWithFrame:frame inView:controlView];
    // nothing draw, transparent background
}

- (void)drawImage:(NSImage *)image withFrame:(NSRect)frame inView:(NSView *)controlView
{
    HoverButton* button = (HoverButton*)self.controlView;
    
    NSRect rc = controlView.bounds;
    NSRect rcImage = NSMakeRect((rc.size.width - image.size.width) / 2.0f, (rc.size.height - image.size.height) / 2.0f, image.size.width, image.size.height);

    double fraction = 0.4;
    if (button.isHovering) fraction = 0.8;
    
    BOOL flipped = self.controlView.flipped;
    [image drawInRect:rcImage fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:fraction respectFlipped:flipped hints:nil];
}

@end
