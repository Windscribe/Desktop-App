#import <Cocoa/Cocoa.h>
#import "ImageResources.h"

NS_ASSUME_NONNULL_BEGIN

@interface ForegroundView : NSImageView {
    ImageResources *imageResources_;
    BOOL isInstallScreen_;
    NSString *caption_;
    NSRect installButtonRect_;
    BOOL bgOpacity_;
}

@property (atomic, strong) NSString *caption;
@property (atomic, assign) BOOL isInstallScreen;
@property (atomic, strong) ImageResources *imageResources;
@property (atomic, assign) NSRect installButtonRect;
@property (atomic, assign) BOOL bgOpacity;

@end

NS_ASSUME_NONNULL_END
