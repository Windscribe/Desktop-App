#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface HoverButton : NSButton
{
    NSTrackingRectTag trackingRect;
}

@property(readonly) BOOL isHovering;

@end

NS_ASSUME_NONNULL_END
