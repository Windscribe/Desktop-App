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
    if (installedHelperJobData)
    {
        NSString*       installedPath           = [[installedHelperJobData objectForKey:@"ProgramArguments"] objectAtIndex:0];
        NSURL*          installedPathURL        = [NSURL fileURLWithPath:installedPath];

        NSDictionary*   installedInfoPlist      = (__bridge NSDictionary*)CFBundleCopyInfoDictionaryForURL( (CFURLRef)installedPathURL );
        NSString*       installedBundleVersion  = [installedInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       installedVersion        = [installedBundleVersion integerValue];

        //qCDebug(LOG_BASIC) << "installed helper version: " << (long)installedVersion;

        NSBundle*       appBundle       = [NSBundle mainBundle];
        NSURL*          appBundleURL    = [appBundle bundleURL];

        NSURL*          currentHelperToolURL    = [appBundleURL URLByAppendingPathComponent:@"Contents/Library/LaunchServices/com.windscribe.helper.macos"];
        NSDictionary*   currentInfoPlist        = (__bridge NSDictionary*)CFBundleCopyInfoDictionaryForURL( (CFURLRef)currentHelperToolURL );
        NSString*       currentBundleVersion    = [currentInfoPlist objectForKey:@"CFBundleVersion"];
        NSInteger       currentVersion          = [currentBundleVersion integerValue];

        //qCDebug(LOG_BASIC) << "current helper version: " << (long)currentVersion;

        if (installedVersion >= currentVersion)
        {
            return true;
        }
    }
    else
    {
        //qCDebug(LOG_BASIC) << "Not installed helper";
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
    if (status != errAuthorizationSuccess)
    {
        //qCDebug(LOG_BASIC) << "Failed to create AuthorizationRef. Error code:" << (int)status;
    }
    else
    {
        // This does all the work of verifying the helper tool against the application
        // and vice-versa. Once verification has passed, the embedded launchd.plist
        // is extracted and placed in /Library/LaunchDaemons and then loaded. The
        // executable is placed in /Library/PrivilegedHelperTools.
        //
        CFErrorRef outError = NULL;
        result = SMJobBless(kSMDomainSystemLaunchd, (__bridge CFStringRef)helperLabel, authRef, &outError);
        if (outError)
        {
            NSError *error = (__bridge NSError *)outError;
            //qCDebug(LOG_BASIC) << QString::fromCFString((CFStringRef)[error localizedDescription]);
            CFRelease(outError);
        }
    }

    return result == YES;
}


