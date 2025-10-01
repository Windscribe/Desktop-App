#include "launchonstartup_mac.h"
#include <AppKit/AppKit.h>
#include <ServiceManagement/ServiceManagement.h>
#include "utils/log/categories.h"
#include "names.h"


bool LaunchOnStartup_mac::isLaunchOnStartupEnabled()
{
    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    NSArray *apps = [workspace runningApplications];

    bool isRunning = false;
    for (NSRunningApplication *a in apps)
    {
        if ([a.bundleIdentifier isEqualToString:@LAUNCHER_BUNDLE_ID])
        {
            isRunning = true;
            break;
        }
    }
    return isRunning;
}


void LaunchOnStartup_mac::setLaunchOnStartup(bool enable)
{
    bool success = SMLoginItemSetEnabled((__bridge CFStringRef)@LAUNCHER_BUNDLE_ID, enable);

    if (success)
    {
        qCInfo(LOG_LAUNCH_ON_STARTUP) << "Successfully updated Launch on startup mode: " << enable;
    }
    else
    {
        qCCritical(LOG_LAUNCH_ON_STARTUP) << "Couldn't update launch on startup mode";
    }
}


