#include "helper_security.h"

#import <Foundation/Foundation.h>
#include <spdlog/spdlog.h>
#include "utils/executable_signature/executable_signature_defs.h"

#define REQUIREMENT_STRING MACOS_DEVID_REQUIREMENT \
                           " and (identifier \"" WS_MAC_GUI_BUNDLE_ID "\" or identifier \"" WS_MAC_INSTALLER_BUNDLE_ID "\")"

namespace HelperSecurity {
static thread_local SecCodeRef tls_currentSecCode_ = NULL;
}

bool HelperSecurity::isValidXpcConnection(xpc_object_t event)
{
    // Always clear any stale ref from a previous call on this thread.
    clearCurrentCallerSecCode();

    // Extract the caller's SecCode regardless of whether we enforce signature
    // verification on this build. Command handlers (e.g. setInstallerPaths)
    // rely on it to resolve the calling bundle's path. Dev builds without
    // USE_SIGNATURE_CHECK get this side benefit without enforcement.
    SecCodeRef secCode = NULL;
    OSStatus osStatus = SecCodeCreateWithXPCMessage(event, kSecCSDefaultFlags, &secCode);
    if (osStatus == errSecSuccess && secCode) {
        tls_currentSecCode_ = secCode;
    }

#if defined(USE_SIGNATURE_CHECK)
    if (!tls_currentSecCode_) {
        spdlog::error("SecCodeCreateWithXPCMessage failed with: {}", (int)osStatus);
        return false;
    }

    SecRequirementRef secRequirement;
    osStatus = SecRequirementCreateWithString(CFSTR(REQUIREMENT_STRING), kSecCSDefaultFlags, &secRequirement);
    if (osStatus != errSecSuccess) {
        spdlog::error("SecRequirementCreateWithString failed with: {}", (int)osStatus);
        clearCurrentCallerSecCode();
        return false;
    }
    bool isValid = SecCodeCheckValidity(tls_currentSecCode_, kSecCSDefaultFlags, secRequirement) == errSecSuccess;
    CFRelease(secRequirement);

    if (!isValid) {
        clearCurrentCallerSecCode();
    }
    return isValid;
#else
    (void)event;
    return true;
#endif
}

SecCodeRef HelperSecurity::currentCallerSecCode()
{
    return tls_currentSecCode_;
}

void HelperSecurity::clearCurrentCallerSecCode()
{
    if (tls_currentSecCode_) {
        CFRelease(tls_currentSecCode_);
        tls_currentSecCode_ = NULL;
    }
}

bool HelperSecurity::recheckCurrentCaller()
{
#if defined(USE_SIGNATURE_CHECK)
    if (!tls_currentSecCode_) {
        return false;
    }
    SecRequirementRef req;
    OSStatus s = SecRequirementCreateWithString(CFSTR(REQUIREMENT_STRING), kSecCSDefaultFlags, &req);
    if (s != errSecSuccess) {
        return false;
    }
    bool ok = SecCodeCheckValidity(tls_currentSecCode_, kSecCSDefaultFlags, req) == errSecSuccess;
    CFRelease(req);
    return ok;
#else
    return true;
#endif
}

