#include "helper_security.h"
#include "logger.h"
#include "utils.h"
#include <boost/algorithm/string.hpp>
#import <Foundation/Foundation.h>

void HelperSecurity::reset()
{
    pid_validity_cache_.clear();
}

bool HelperSecurity::verifyProcessId(pid_t pid)
{
    const auto it = pid_validity_cache_.find(pid);
    if (it != pid_validity_cache_.end())
        return it->second;

    LOG("HelperSecurity::verifyProcessId: new PID %u", static_cast<unsigned int>(pid));
    return verifyProcessIdImpl(pid);
}

bool HelperSecurity::verifyProcessIdImpl(pid_t pid)
{
    // Get application name from pid.
    std::string app_name;
    Utils::executeCommand("ps awx | awk '$1 == " + std::to_string(pid) + " { print $5 }'", {}, &app_name);
    boost::trim(app_name);
    if (app_name.empty()) {
        LOG("Failed to get app/bundle name for PID %i", pid);
        return false;
    }

    bool result = false;
    
    
    std::vector<std::string> endings;

    // Check for a correct ending.
    endings.push_back("/Contents/MacOS/installer");
#if DEBUG || DISABLE_HELPER_SECURITY_CHECK
    endings.push_back("/WindscribeEngine");
#else
    endings.push_back("/WindscribeEngine.app/Contents/MacOS/WindscribeEngine");
#endif
    
    const auto app_name_length = app_name.length();
    do {
        // Check bundle name.
        bool bFoundBundleName = false;
        for (auto ending : endings)
        {
            const auto ending_length = ending.length();
            if (app_name_length >= ending_length &&
                app_name.compare(app_name_length - ending_length, ending_length, ending) == 0)
            {
                bFoundBundleName = true;
                break;
            }
        }
        if (!bFoundBundleName)
        {
            LOG("Invalid app/bundle name for PID %i: '%s'", pid, app_name.c_str());
            break;
        }
#if DEBUG || DISABLE_HELPER_SECURITY_CHECK
        result = true;
#else
        // Check code signature.
        SecStaticCodeRef staticCode = NULL;
        NSString* path = [NSString stringWithCString:app_name.c_str()
                                   encoding:[NSString defaultCStringEncoding]];
        OSStatus status = SecStaticCodeCreateWithPath(
            (__bridge CFURLRef)([NSURL fileURLWithPath:path]), kSecCSDefaultFlags, &staticCode);
        if (status != errSecSuccess) {
            LOG("Invalid signature for PID %i (0)", pid);
            break;
        }
        CFDictionaryRef signingDetails = NULL;
        status = SecCodeCopySigningInformation(staticCode, kSecCSSigningInformation, &signingDetails);
        if (status != errSecSuccess) {
            LOG("Invalid signature for PID %i (1)", pid);
            break;
        }
        NSArray *certificateChain = [((__bridge NSDictionary*)signingDetails)
                                     objectForKey: (__bridge NSString*)kSecCodeInfoCertificates];
        if (certificateChain.count == 0) {
            LOG("Invalid signature for PID %i (2)", pid);
            break;
        }
        CFStringRef commonName = NULL;
        for (NSUInteger i = 0; i < certificateChain.count; ++i) {
            SecCertificateRef certificate =
                (__bridge SecCertificateRef)([certificateChain objectAtIndex:i]);
            if (errSecSuccess == SecCertificateCopyCommonName(certificate, &commonName) && commonName) {
                if (CFEqual((CFTypeRef)commonName,
                            (CFTypeRef)@"Developer ID Application: Windscribe Limited (GYZJYS7XUG)")) {
                    result = true;
                    break;
                }
             }
        }
        if (!result)
            LOG("No matching certificate for PID %i", pid);
#endif
    } while (0);

    LOG("Verification result for PID %i: %s", pid, result ? "SUCCEEDED" : "FAILED");
    pid_validity_cache_[pid] = result;
    return result;
}
