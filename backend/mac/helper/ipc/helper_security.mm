#include "helper_security.h"

#import <Foundation/Foundation.h>
#include <spdlog/spdlog.h>
#include "utils/executable_signature/executable_signature_defs.h"

#define REQUIREMENT_STRING "anchor apple generic and (identifier \"com.windscribe.gui.macos\" or identifier \"com.windscribe.installer.macos\")" \
                           "and certificate leaf[subject.CN] = \"" MACOS_CERT_DEVELOPER_ID "\""

bool HelperSecurity::isValidXpcConnection(xpc_object_t event)
{
#if defined(USE_SIGNATURE_CHECK)
    SecCodeRef secCode;
    OSStatus osStatus = SecCodeCreateWithXPCMessage(event, kSecCSDefaultFlags, &secCode);
    if (osStatus != errSecSuccess) {
        spdlog::error("SecCodeCreateWithXPCMessage failed with: {}", (int)osStatus);
        return false;
    }

    CFStringRef requirementString = CFSTR(REQUIREMENT_STRING);
    SecRequirementRef secRequirement;
    osStatus = SecRequirementCreateWithString(requirementString, kSecCSDefaultFlags, &secRequirement);
    if (osStatus != errSecSuccess) {
        spdlog::error("SecRequirementCreateWithString failed with: {}", (int)osStatus);
        CFRelease(secCode);
        return false;
    }
    bool isValid =  SecCodeCheckValidity(secCode, kSecCSDefaultFlags,  secRequirement) == errSecSuccess;
    CFRelease(secRequirement);
    CFRelease(secCode);

    return isValid;
#endif
  (void)event;
  return true;
}

