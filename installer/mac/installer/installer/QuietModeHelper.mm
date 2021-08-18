#import <Foundation/Foundation.h>
#import "QuietModeHelper.h"

#include "installhelper_mac.h"
#import "Logger.h"

@implementation QuietModeHelper

- (id)initWithPath:(NSString *)path andDelegate:(AppDelegate *)appDelegate
{
    self = [super init];

    if (self)
    {
        _appDelegate = appDelegate;
        [_appDelegate.installer setObjectForCallback:@selector(installerCallback:) withObject:self];
    }
    return self;
}

- (void) start
{
    [[Logger sharedLogger] logAndStdOut:@"QuietModeHelper starting"];

    //install helper
    if (!InstallHelper_mac::installHelper())
    {
        [[Logger sharedLogger] logAndStdOut:@"Failed to install helper"];
        [NSApp terminate:self];
        return;
    }
    
    // start installation
    [_appDelegate.installer start:false];
}

- (void) installerCallback: (id)object
{
    BaseInstaller *installer = object;
    if (installer.currentState == STATE_EXTRACTING)
    {
        // NSLog(@"Progress updated");

    }
    else if (installer.currentState == STATE_ERROR)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        NSString *errStr = @"Error during installation.";
        [alert setMessageText:errStr];
        [[Logger sharedLogger] logAndStdOut:errStr];

        [alert setInformativeText: installer.lastError];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert runModal];
    }
    else if (installer.currentState == STATE_FINISHED)
    {
        static bool isAlreadyFinished = false;
        // for prevent duplicate run
        if (!isAlreadyFinished)
        {
            [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Finished! (%@)", [installer getFullInstallPath]]];

            [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(appDidLaunch:) name:NSWorkspaceDidLaunchApplicationNotification object:nil];
            [[NSWorkspace sharedWorkspace] launchApplication: [installer getFullInstallPath]];
            isAlreadyFinished = true;
        }
    }
}

- (void)appDidLaunch:(NSNotification*)note
{
    [NSApp terminate:self];
}


@end
