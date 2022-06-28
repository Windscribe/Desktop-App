#import "ImageResources.h"
#import "ToggleControl.h"
#import <QuartzCore/QuartzCore.h>

@interface ToggleControl () {
    __weak id _target;
    SEL _action;
}

@property (nonatomic, readonly, strong) CALayer *bgLayer;
@property (nonatomic, readonly, strong) CALayer *indicatorLayer;
@property (nonatomic, assign) ImageResources *resources;

@end

@implementation ToggleControl

- (id)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    [self setup];
    return self;
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (!self) {
        return nil;
    }

    [self setup];
    return self;
}

- (void)setup {
    _bgLayer = [CALayer layer];
    self.layer = _bgLayer;
    self.wantsLayer = YES;
    [self.layer addSublayer:_indicatorLayer];
    _indicatorLayer = [CALayer layer];
    _indicatorLayer.frame = [self getIndicatorRect];
    [self.layer addSublayer:_indicatorLayer];
}

- (void)setImageResources:(ImageResources *)resources {
    _resources = resources;
    _indicatorLayer.contents = _resources.toggleIndicator.NSImage;
    _indicatorLayer.contentsGravity = kCAGravityResizeAspect;
    [self reloadLayer];
}

- (void)reloadLayer {
    [CATransaction begin];
    [CATransaction setAnimationDuration:0.2f];
    {
        if ([self checked]) {
            _bgLayer.contents = _resources.toggleBgGreen.NSImage;
        } else {
            _bgLayer.contents = _resources.toggleBgWhite.NSImage;
        }
        self.indicatorLayer.frame = [self getIndicatorRect];
    }
    [CATransaction commit];
}

- (CGRect)getIndicatorRect {
    return (CGRect) {
        .size.width = _resources.toggleIndicator.size.width,
        .size.height = _resources.toggleIndicator.size.height,
        .origin.x = ![self checked] ? 2 : NSWidth(_bgLayer.bounds) - _resources.toggleIndicator.size.width - 2,
        .origin.y = NSHeight(_bgLayer.bounds)/2 - _resources.toggleIndicator.size.height/2
    };
}

- (void)mouseUp:(NSEvent *)event {
    self.checked = ![self checked];
    [self reloadLayer];
    [NSApp sendAction:self.action to:self.target from:self];
}

- (id)target {
    return _target;
}

- (void)setTarget:(id)target {
    _target = target;
}

- (SEL)action {
    return _action;
}

- (void)setAction:(SEL)action {
    _action = action;
}

@end