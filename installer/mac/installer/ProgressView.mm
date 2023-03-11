#import "ProgressView.h"
#import <CoreImage/CoreImage.h>

@implementation ProgressView

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        progressIndicator_ = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0, 0, 16, 16)];
        progressIndicator_.style = NSProgressIndicatorStyleSpinning;
        [self setProgressIndicatorColor: [NSColor colorWithCalibratedRed: 0.0 green: 0.03 blue: 0.1 alpha: 1]];
        [self addSubview:progressIndicator_];
        
        label_ = [[NSTextField alloc] initWithFrame:NSMakeRect(50, 50, 25, 25)];
        label_.stringValue = [NSString stringWithFormat:@"%d%@", 0, @"%"];
        label_.editable = NO;
        label_.bordered = NO;
        label_.alignment = NSTextAlignmentCenter;
        [label_ setFont:[NSFont fontWithName:@"IBM Plex Sans" size:12]];
        label_.textColor = [NSColor colorWithCalibratedRed: 0.0 green: 0.03 blue: 0.1 alpha: 1];
        label_.drawsBackground = NO;
        [self addSubview:label_];
        
        [self updateChildsPosition];
        
        progress_ = 0;
    }
    return self;
}

- (void)setProgressIndicatorColor:(NSColor *)color
{
    CGFloat tintColorRedComponent = color.redComponent;
    CGFloat tintColorGreenComponent = color.greenComponent;
    CGFloat tintColorBlueComponent = color.blueComponent;
 
    CIVector *tintColorMinComponentsVector = [CIVector vectorWithX:tintColorRedComponent Y:tintColorGreenComponent Z:tintColorBlueComponent W:0.0 ];
    CIVector *tintColorMaxComponentsVector = [CIVector vectorWithX:tintColorRedComponent Y:tintColorGreenComponent Z:tintColorBlueComponent W:1.0 ];

    CIFilter *colorClampFilter = [CIFilter filterWithName: @"CIColorClamp"];
    [colorClampFilter setDefaults];
    [colorClampFilter setValue:tintColorMinComponentsVector forKey:@"inputMinComponents"];
    [colorClampFilter setValue:tintColorMaxComponentsVector forKey:@"inputMaxComponents"];
    NSArray *arr = [NSArray arrayWithObject:colorClampFilter];
    [progressIndicator_ setContentFilters:arr];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    
    [[NSGraphicsContext currentContext] saveGraphicsState];

    NSColor* fillColor;

    fillColor = [NSColor colorWithCalibratedRed: 0.95 green: 0.95 blue: 0.95 alpha: 1];

    [fillColor setStroke];
    [fillColor setFill];
    NSBezierPath* roundedPath = [NSBezierPath bezierPathWithRoundedRect:[self bounds] xRadius:16 yRadius:16];
    [roundedPath stroke];
    [roundedPath fill];
    
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)updateChildsPosition
{
    NSInteger TEXT_MARGIN = 10;
    NSRect rc = [self bounds];
    [label_ setFrame:NSMakeRect(0, 0, 50, 50)];
    [progressIndicator_ setFrame:NSMakeRect(rc.size.width - 16 - TEXT_MARGIN, (rc.size.height - 16) / 2.0, 16, 16)];
    
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
    [progressIndicator_ startAnimation:nil];
}
- (void)stopAnimation
{
    [progressIndicator_ stopAnimation:nil];
}
- (void)setProgress:(int) value
{
    progress_ = value;
    label_.stringValue = [NSString stringWithFormat:@"%d%@", progress_, @"%"];
}

@end
