#include "checkrunningapp_mac.h"
#include <AppKit/AppKit.h>

void CheckRunningApp_mac::checkPrevInstance(bool &bShouldCloseCurrentApp)
{
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier: @"com.aaa.windscribe.windscribe"];

    if ([apps count] == 0)
    {
        bShouldCloseCurrentApp = false;
    }
    else
    {
        pid_t curPid = getpid();
        if ([apps count] == 1)
        {
            if ([apps[0] processIdentifier] == curPid)
            {
                bShouldCloseCurrentApp = false;
                return;
            }
        }

        for (NSUInteger i = 0; i < [apps count]; i++)
        {
            if ([apps[i] processIdentifier] != curPid)
            {
                if (![apps[0] activateWithOptions: NSApplicationActivateIgnoringOtherApps])
                {
                    // failed to activate;
                    bShouldCloseCurrentApp = true;
                }
                else
                {
                    bShouldCloseCurrentApp = true;
                }
                break;
            }
        }

    }
}

