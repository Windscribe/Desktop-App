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
    for (NSRunningApplication *a in apps) {
        NSString *str = [NSString stringWithFormat:@"%s", GUI_BUNDLE_ID];
        if ([a.bundleIdentifier isEqualToString:str]) {
            isRunning = true;
            break;
        }
    }

    // run if not already running
    if (!isRunning) {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        for (int i = 0; i < 4; ++i) {
            bundlePath = [bundlePath stringByDeletingLastPathComponent];
        }

        NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
        configuration.arguments = @[@"--autostart"];

        [workspace openApplicationAtURL:[NSURL fileURLWithPath:bundlePath] configuration:configuration completionHandler:nil];
    }
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {

}


@end
