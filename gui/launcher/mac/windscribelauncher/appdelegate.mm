#import "appdelegate.h"
#include "names.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSArray *apps = [workspace runningApplications];
    
    // determine if already running
    bool isRunning = false;
    for (NSRunningApplication *a in apps)
    {
        NSString *str = [NSString stringWithUTF8String:GUI_BUNDLE_ID.c_str()];
        if ([a.bundleIdentifier isEqualToString:str]) {
            isRunning = true;
            break;
        }
    }
    
    // run if not already running
    if (!isRunning)
    {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        for (int i = 0; i < 4; ++i)
        {
            bundlePath = [bundlePath stringByDeletingLastPathComponent];
        }
        
        [workspace launchApplication:bundlePath];
    }
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {

}


@end
