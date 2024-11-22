#include "installhelper_mac.h"

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#import <Security/Authorization.h>

#include <spdlog/spdlog.h>
#include "../string_utils.h"
#include "../../../../client/common/version/windscribe_version.h"


bool InstallHelper_mac::installHelper(bool bForceDeleteOld)
{
    NSString *helperLabel = @"com.windscribe.helper.macos";
    BOOL result = NO;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSDictionary *installedHelperJobData  = CFBridgingRelease(SMJobCopyDictionary(kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel));
#pragma clang diagnostic pop
    if (installedHelperJobData) {
        NSString*       installedPath           = [[installedHelperJobData objectForKey:@"ProgramArguments"] objectAtIndex:0];
        NSURL*          installedPathURL        = [NSURL fileURLWithPath:installedPath];

        NSDictionary*   installedInfoPlist      = CFBridgingRelease(CFBundleCopyInfoDictionaryForURL((CFURLRef)installedPathURL));
        NSString*       installedBundleVersion  = [installedInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       installedVersion        = [installedBundleVersion integerValue];

        spdlog::info("InstallHelper - installed helper version: {}", (long)installedVersion);

        NSBundle*       appBundle       = [NSBundle mainBundle];
        NSURL*          appBundleURL    = [appBundle bundleURL];

        NSURL*          currentHelperToolURL    = [appBundleURL URLByAppendingPathComponent:@"Contents/Library/LaunchServices/com.windscribe.helper.macos"];
        NSDictionary*   currentInfoPlist        = CFBridgingRelease(CFBundleCopyInfoDictionaryForURL((CFURLRef)currentHelperToolURL));
        NSString*       currentBundleVersion    = [currentInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       currentVersion          = [currentBundleVersion integerValue];

        spdlog::info("InstallHelper - current helper version: {}", (long)currentVersion);

        if (installedVersion == currentVersion && isAppMajorMinorVersionSame() && !bForceDeleteOld) {
            return true;
        } else if (installedVersion >= currentVersion || bForceDeleteOld) {
            // If we are downgrading (or the helper version is the same but app version differs),
            // we need to uninstall the previous helper (SMJobBless will not let us downgrade)
            spdlog::info("Force reinstalling helper.");
            NSString *scriptContents = @"do shell script \"launchctl remove /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                          "rm /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                          "rm /Library/PrivilegedHelperTools/com.windscribe.helper.macos\" with administrator privileges";
            NSAppleScript *script = [[NSAppleScript alloc] initWithSource:scriptContents];
            NSAppleEventDescriptor *desc;
            desc = [script executeAndReturnError:nil];
            if (desc == nil) {
                spdlog::error("Couldn't remove the previous helper.");
            } else {
                spdlog::info("Removed previous helper.");
            }
        }
    } else {
        spdlog::error("InstallHelper - helper not installed");
    }

    AuthorizationItem authItem     = { kSMRightBlessPrivilegedHelper, 0, NULL, 0 };
    AuthorizationRights authRights = { 1, &authItem };
    AuthorizationFlags flags       = kAuthorizationFlagDefaults |
                                     kAuthorizationFlagInteractionAllowed |
                                     kAuthorizationFlagPreAuthorize |
                                     kAuthorizationFlagExtendRights;

    AuthorizationRef authRef = NULL;

    // Obtain the right to install privileged helper tools (kSMRightBlessPrivilegedHelper).
    OSStatus status = AuthorizationCreate(&authRights, kAuthorizationEmptyEnvironment, flags, &authRef);
    if (status != errAuthorizationSuccess) {
        spdlog::error("InstallHelper - failed to create AuthorizationRef. Error code: {}", (int)status);
    } else {
        // This does all the work of verifying the helper tool against the application
        // and vice-versa. Once verification has passed, the embedded launchd.plist
        // is extracted and placed in /Library/LaunchDaemons and then loaded. The
        // executable is placed in /Library/PrivilegedHelperTools.
        //
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CFErrorRef outError = NULL;
        result = SMJobBless(kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel, authRef, &outError);
#pragma clang diagnostic pop
        if (!result) {
            if (outError) {
                spdlog::error("InstallHelper - SMJobBless failed. Error code: {}", CFErrorGetCode(outError));
                spdlog::error("Description: {}", toStdString(CFErrorCopyDescription(outError)));
                spdlog::error("Reason: {}", toStdString(CFErrorCopyFailureReason(outError)));
                spdlog::error("Recovery: {}", toStdString(CFErrorCopyRecoverySuggestion(outError)));
                CFRelease(outError);
            } else {
                spdlog::error("InstallHelper - SMJobBless failed. No error info available.");
            }
        }
    }

    return result == YES;
}

bool InstallHelper_mac::isAppMajorMinorVersionSame()
{
    CFURLRef url = CFURLCreateWithString(NULL, CFSTR("/Applications/Windscribe.app"), NULL);
    if (url == NULL) {
        return false;
    }

    CFBundleRef bundle = CFBundleCreate(NULL, url);
    CFRelease(url);

    if (bundle == NULL) {
        return false;
    }

    CFStringRef version = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
    CFRelease(bundle);

    if (version && CFStringHasPrefix(version, CFSTR(WINDSCRIBE_MAJOR_MINOR_VERSION_STR))) {
        return true;
    }

    return false;
}
