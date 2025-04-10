#include "installhelper_mac.h"

#include "../../../common/version/windscribe_version.h"
#include "names.h"

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#import <Security/Authorization.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {
  std::string toStdString(CFStringRef str)
  {
      std::string res;
      if (str)
          res = [(__bridge NSString *)str UTF8String];
      return res;
  }
}

bool InstallHelper_mac::installHelper(bool bForceDeleteOld, bool &isUserCanceled, spdlog::logger *logger)
{
    isUserCanceled = false;
    NSString *helperLabel = @HELPER_BUNDLE_ID;
    BOOL result = NO;

    NSDictionary *installedHelperJobData  = CFBridgingRelease(SMJobCopyDictionary(kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel));

    if (installedHelperJobData) {
        NSString*       installedPath           = [[installedHelperJobData objectForKey:@"ProgramArguments"] objectAtIndex:0];
        NSURL*          installedPathURL        = [NSURL fileURLWithPath:installedPath];

        NSDictionary*   installedInfoPlist      = CFBridgingRelease(CFBundleCopyInfoDictionaryForURL((CFURLRef)installedPathURL));
        NSString*       installedBundleVersion  = [installedInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       installedVersion        = [installedBundleVersion integerValue];

        logger->info("Installed helper version: {}", (long)installedVersion);

        NSBundle*       appBundle       = [NSBundle mainBundle];
        NSURL*          appBundleURL    = [appBundle bundleURL];

        NSURL*          currentHelperToolURL    = [appBundleURL URLByAppendingPathComponent:@HELPER_BUNDLE_ID_PATH_FROM_ENGINE];
        NSDictionary*   currentInfoPlist        = CFBridgingRelease(CFBundleCopyInfoDictionaryForURL((CFURLRef)currentHelperToolURL));
        NSString*       currentBundleVersion    = [currentInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       currentVersion          = [currentBundleVersion integerValue];

        logger->info("Current helper version: {}", (long)currentVersion);

        if (installedVersion == currentVersion && isAppMajorMinorVersionSame() && !bForceDeleteOld) {
            return true;
        } else if (installedVersion >= currentVersion || bForceDeleteOld) {
            // If we are downgrading, (or the helper version is the same but app version differs),
            // we need to uninstall the previous helper first (SMJobBless will not let us downgrade)
            uninstallHelper(logger);
        }
    } else {
        logger->info("Helper not installed");
    }

    AuthorizationItem authItem      = { kSMRightBlessPrivilegedHelper, 0, NULL, 0 };
    AuthorizationRights authRights  = { 1, &authItem };
    AuthorizationFlags flags        = kAuthorizationFlagDefaults |
                                      kAuthorizationFlagInteractionAllowed |
                                      kAuthorizationFlagPreAuthorize |
                                      kAuthorizationFlagExtendRights;

    AuthorizationRef authRef = NULL;

    // Obtain the right to install privileged helper tools (kSMRightBlessPrivilegedHelper).
    OSStatus status = AuthorizationCreate(&authRights, kAuthorizationEmptyEnvironment, flags, &authRef);
    if (status != errAuthorizationSuccess) {
        logger->warn("Failed to create AuthorizationRef. Error code: {}", (int)status);
        isUserCanceled = true;
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
                logger->error("InstallHelper - SMJobBless failed. Error code: {}", CFErrorGetCode(outError));
                logger->error("Description: {}", toStdString(CFErrorCopyDescription(outError)));
                logger->error("Reason: {}", toStdString(CFErrorCopyFailureReason(outError)));
                logger->error("Recovery: {}", toStdString(CFErrorCopyRecoverySuggestion(outError)));
                CFRelease(outError);
            } else {
                logger->error("InstallHelper - SMJobBless failed. No error info available.");
            }
        }
    }

    return result == YES;
}

bool InstallHelper_mac::uninstallHelper(spdlog::logger *logger)
{
    logger->info("Uninstalling helper");
    NSString *scriptContents = @"do shell script \"launchctl remove /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                  "rm /Library/LaunchDaemons/com.windscribe.helper.macos.plist;"
                                                  "rm /Library/PrivilegedHelperTools/com.windscribe.helper.macos\" with administrator privileges";
    NSAppleScript *script = [[NSAppleScript alloc] initWithSource:scriptContents];
    NSAppleEventDescriptor *desc;
    desc = [script executeAndReturnError:nil];
    if (desc == nil) {
        logger->error("Couldn't remove the previous helper");
        return true;
    } else {
        logger->info("Removed previous helper");
        return false;
    }
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

#pragma clang diagnostic pop
