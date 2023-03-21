#import "ProgressView.h"

@implementation ProgressView

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        angle_ = 0;

        label_ = [[NSTextField alloc] initWithFrame:NSMakeRect(50, 50, 25, 25)];
        label_.stringValue = [NSString stringWithFormat:@"%d%@", 0, @"%"];
        label_.editable = NO;
        label_.bordered = NO;
        label_.alignment = NSTextAlignmentCenter;
        [label_ setFont:[NSFont fontWithName:@"IBM Plex Sans" size:16]];
        label_.textColor = [NSColor colorWithCalibratedRed: 0.0 green: 0.03 blue: 0.1 alpha: 1];
        label_.drawsBackground = NO;
        [self addSubview:label_];

        [self updateChildsPosition];

        progress_ = 0;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    [[NSGraphicsContext currentContext] saveGraphicsState];

    NSColor* fillColor = [NSColor colorWithCalibratedRed: 0.95 green: 0.95 blue: 0.95 alpha: 1];
    [fillColor setFill];

    CGPoint spinnerCenter;
    NSRect rc = [self bounds];
    if (progress_ < 0) {
        spinnerCenter.x = rc.size.width/2;
        spinnerCenter.y = rc.size.height/2;

        rc.origin.x = rc.origin.x + rc.size.width/2 - 22;
        rc.size.width = 44;

        NSBezierPath* circlePath = [NSBezierPath bezierPathWithOvalInRect: rc];
        [circlePath fill];
    } else {
        spinnerCenter.x = rc.size.width - 22;
        spinnerCenter.y = rc.size.height/2;

        NSBezierPath* roundedPath = [NSBezierPath bezierPathWithRoundedRect:[self bounds] xRadius:22 yRadius:22];
        [roundedPath fill];
    }

    NSColor* spinnerColor = [NSColor colorWithCalibratedRed: 0.01 green: 0.05 blue: 0.11 alpha: 1];
    [spinnerColor setStroke];

    NSBezierPath* spinnerPath = [NSBezierPath bezierPath];
    spinnerPath.lineWidth = 2;
    [spinnerPath appendBezierPathWithArcWithCenter: spinnerCenter
                                            radius: 8
                                        startAngle: angle_
                                          endAngle: angle_ + 270
                                         clockwise: NO];
    [spinnerPath stroke];

    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)updateChildsPosition
{
    NSInteger TEXT_MARGIN = 10;
    NSRect rc = [self bounds];

    [label_ setFrame:NSMakeRect(0, 0, 50, 50)];
    [label_ sizeToFit];

    NSRect rcLabel = [label_ bounds];
    [label_ setFrame:NSMakeRect(TEXT_MARGIN, (rc.size.height - rcLabel.size.height) / 2.0 + 1, rc.size.width-TEXT_MARGIN*2-16, rcLabel.size.height)];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
    [super resizeSubviewsWithOldSize:oldSize];
    [self updateChildsPosition];
}

- (void)startAnimation
{
    timer_ = [NSTimer scheduledTimerWithTimeInterval: 0.01
                                              target: self
                                            selector: @selector(onTimer)
                                            userInfo: nil
                                             repeats: YES];
}

- (void)stopAnimation
{
    [timer_ invalidate];
    timer_ = nil;
}

- (void)onTimer
{
    angle_ -= 6;
    self.needsDisplay = YES;
}


- (void)setProgress:(int) value
{
    progress_ = value;

    if (progress_ < 0) {
        // app is launching, remove label
        [label_ removeFromSuperview];

        // Refresh
        self.needsDisplay = YES;
    } else {
        label_.stringValue = [NSString stringWithFormat:@"%d%@", progress_, @"%"];
    }
}

@end