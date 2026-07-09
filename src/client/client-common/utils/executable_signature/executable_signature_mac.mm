#include "executable_signature_mac.h"

#import <Foundation/Foundation.h>

#include "executable_signature.h"
#include "executable_signature_defs.h"
#include "utils/wsscopeguard.h"

ExecutableSignaturePrivate::ExecutableSignaturePrivate(ExecutableSignature* const q) : ExecutableSignaturePrivateBase(q)
{
}

ExecutableSignaturePrivate::~ExecutableSignaturePrivate()
{
}

bool ExecutableSignaturePrivate::verify(const std::wstring& exePath)
{
    NSString* path = [[NSString alloc] initWithBytes:exePath.data()
                                              length:exePath.size() * sizeof(wchar_t)
                                            encoding:NSUTF32LittleEndianStringEncoding];
    if (path == nil) {
        lastError_ << "Failed to convert wide path to UTF-8";
        return false;
    }
    return verify(std::string([path UTF8String]));
}

bool ExecutableSignaturePrivate::verify(const std::string &exePath)
{
    SecStaticCodeRef staticCode = NULL;
    SecRequirementRef requirement = NULL;

    auto exitGuard = wsl::wsScopeGuard([&] {
        if (requirement != NULL) CFRelease(requirement);
        if (staticCode != NULL) CFRelease(staticCode);
    });

    NSString* path = [NSString stringWithUTF8String:exePath.c_str()];
    if (path == NULL) {
        lastError_ << "Failed to convert path to NSString";
        return false;
    }

    OSStatus status = SecStaticCodeCreateWithPath((__bridge CFURLRef)([NSURL fileURLWithPath:path]), kSecCSDefaultFlags, &staticCode);
    if (status != errSecSuccess) {
        lastError_ << "SecStaticCodeCreateWithPath failed: " << status;
        return false;
    }

    // A NULL requirement would accept any internally-consistent seal, including a self-signed one.
    status = SecRequirementCreateWithString(CFSTR(MACOS_DEVID_REQUIREMENT), kSecCSDefaultFlags, &requirement);
    if (status != errSecSuccess) {
        lastError_ << "SecRequirementCreateWithString failed: " << status;
        return false;
    }

    SecCSFlags flags = kSecCSStrictValidate | kSecCSCheckAllArchitectures | kSecCSCheckNestedCode;
    status = SecStaticCodeCheckValidity(staticCode, flags, requirement);
    if (status != errSecSuccess) {
        lastError_ << "SecStaticCodeCheckValidity failed: " << status;
        return false;
    }

    return true;
}
