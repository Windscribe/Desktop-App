#import <Cocoa/Cocoa.h>
#import "installer/base_installer.hpp"

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    BaseInstaller *installer_;
}

@property (nonatomic) BaseInstaller *installer;
@property(atomic, assign) bool isLegacy;
@property(atomic, assign) bool isUpdateMode;

@end

