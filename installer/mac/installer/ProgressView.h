#import <Cocoa/Cocoa.h>
#import "ImageResources.h"

NS_ASSUME_NONNULL_BEGIN

@interface ProgressView : NSView
{
    NSImageView *spinnerView_;
    NSTextField *label_;
    NSTimer *timer_;
    int progress_;
    float angle_;
}

- (void)startAnimation;
- (void)stopAnimation;
- (void)setProgress:(int) value;
- (void)onTimer;

@end

NS_ASSUME_NONNULL_END
