#import "appdelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSArray *apps = [workspace runningApplications];
    
    bool isRunning = false;
    for (NSRunningApplication *a in apps) {
        if ([a.bundleIdentifier isEqualToString:@"com.windscribe.WindscribeGUI"]) {
            isRunning = true;
            break;
        }
    }
    
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
