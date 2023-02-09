#include "installhelper_mac.h"

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#import <Security/Authorization.h>

#import "../Logger.h"

bool InstallHelper_mac::installHelper()
{
    NSString *helperLabel = @"com.windscribe.helper.macos";
    BOOL result = NO;

    NSDictionary *installedHelperJobData  = (__bridge NSDictionary *)SMJobCopyDictionary( kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel);
    if (installedHelperJobData) {
        NSString*       installedPath           = [[installedHelperJobData objectForKey:@"ProgramArguments"] objectAtIndex:0];
        NSURL*          installedPathURL        = [NSURL fileURLWithPath:installedPath];

        NSDictionary*   installedInfoPlist      = (__bridge NSDictionary*)CFBundleCopyInfoDictionaryForURL( (CFURLRef)installedPathURL );
        NSString*       installedBundleVersion  = [installedInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       installedVersion        = [installedBundleVersion integerValue];

        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - installed helper version: %li", (long)installedVersion]];

        NSBundle*       appBundle       = [NSBundle mainBundle];
        NSURL*          appBundleURL    = [appBundle bundleURL];

        NSURL*          currentHelperToolURL    = [appBundleURL URLByAppendingPathComponent:@"Contents/Library/LaunchServices/com.windscribe.helper.macos"];
        NSDictionary*   currentInfoPlist        = (__bridge NSDictionary*)CFBundleCopyInfoDictionaryForURL( (CFURLRef)currentHelperToolURL );
        NSString*       currentBundleVersion    = [currentInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       currentVersion          = [currentBundleVersion integerValue];

        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - current helper version: %li", (long)currentVersion]];

        if (installedVersion == currentVersion) {
            return true;
        } else if (installedVersion > currentVersion) {
            // If we are downgrading, we need to uninstall the previous helper (SMJobBless will not let us downgrade)
            [[Logger sharedLogger] logAndStdOut:@"Downgrading helper."];
            NSString *scriptContents = @"do shell script \"launchctl remove /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                          "rm /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                          "rm /Library/PrivilegedHelperTools/com.windscribe.helper.macos\" with administrator privileges";
            NSAppleScript *script = [[NSAppleScript alloc] initWithSource:scriptContents];
            NSAppleEventDescriptor *desc;
            desc = [script executeAndReturnError:nil];
            if (desc == nil) {
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Couldn't remove the previous helper."]];
            } else {
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Removed previous helper."]];
            }
        }
    } else {
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - helper not installed"]];
    }

    AuthorizationItem authItem		= { kSMRightBlessPrivilegedHelper, 0, NULL, 0 };
    AuthorizationRights authRights	= { 1, &authItem };
    AuthorizationFlags flags		=	kAuthorizationFlagDefaults				|
    kAuthorizationFlagInteractionAllowed	|
    kAuthorizationFlagPreAuthorize			|
    kAuthorizationFlagExtendRights;

    AuthorizationRef authRef = NULL;

    // Obtain the right to install privileged helper tools (kSMRightBlessPrivilegedHelper).
    OSStatus status = AuthorizationCreate(&authRights, kAuthorizationEmptyEnvironment, flags, &authRef);
    if (status != errAuthorizationSuccess) {
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - failed to create AuthorizationRef. Error code: %i", (int)status]];
    } else {
        // This does all the work of verifying the helper tool against the application
        // and vice-versa. Once verification has passed, the embedded launchd.plist
        // is extracted and placed in /Library/LaunchDaemons and then loaded. The
        // executable is placed in /Library/PrivilegedHelperTools.
        //
        CFErrorRef outError = NULL;
        result = SMJobBless(kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel, authRef, &outError);
        if (!result) {
            if (outError) {
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - SMJobBless failed. Error code: %li", CFErrorGetCode(outError)]];
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Description: %@", CFErrorCopyDescription(outError)]];
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Reason: %@", CFErrorCopyFailureReason(outError)]];
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Recovery: %@", CFErrorCopyRecoverySuggestion(outError)]];
                CFRelease(outError);
            } else {
                [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"InstallHelper - SMJobBless failed. No error info available."]];
            }
        }
    }

    return result == YES;
}
