#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface ProgressView : NSView
{
    NSProgressIndicator *progressIndicator_;
    NSTextField *label_;
    NSTimer *timer_;
    int progress_;
}

- (void)startAnimation;
- (void)stopAnimation;
- (void)setProgress:(int) value;

@end

NS_ASSUME_NONNULL_END
