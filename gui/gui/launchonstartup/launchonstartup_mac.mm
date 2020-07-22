#include "launchonstartup_mac.h"
#include <AppKit/AppKit.h>
#include <ServiceManagement/ServiceManagement.h>

bool LaunchOnStartup_mac::isLaunchOnStartupEnabled()
{
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSArray *apps = [workspace runningApplications];

    bool isRunning = false;
    for (NSRunningApplication *a in apps)
    {
        if ([a.bundleIdentifier isEqualToString:@"com.windscribe.WindscribeLauncher"])
        {
            isRunning = true;
            break;
        }
    }
    return isRunning;
}


void LaunchOnStartup_mac::setLaunchOnStartup(bool enable)
{
    SMLoginItemSetEnabled((__bridge CFStringRef)@"com.windscribe.WindscribeLauncher", enable);
}


