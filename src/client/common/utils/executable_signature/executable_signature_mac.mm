#include "executable_signature_mac.h"

#import <Foundation/Foundation.h>

#include <codecvt>

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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string converted = converter.to_bytes(exePath);
    return verify(converted);
#pragma clang diagnostic pop
}

bool ExecutableSignaturePrivate::verify(const std::string &exePath)
{
    SecStaticCodeRef staticCode = NULL;
    CFDictionaryRef signingDetails = NULL;

    auto exitGuard = wsl::wsScopeGuard([&] {
        if (signingDetails != NULL) CFRelease(signingDetails);
        if (staticCode != NULL) CFRelease(staticCode);
    });

    NSString* path = [NSString stringWithCString:exePath.c_str()
                               encoding:[NSString defaultCStringEncoding]];
    if (path == NULL) {
        lastError_ << "Failed to convert path to NSString";
        return false;
    }

    OSStatus status = SecStaticCodeCreateWithPath((__bridge CFURLRef)([NSURL fileURLWithPath:path]), kSecCSDefaultFlags, &staticCode);
    if (status != errSecSuccess) {
        lastError_ << "SecStaticCodeCreateWithPath failed: " << status;
        return false;
    }

    SecCSFlags flags = kSecCSDefaultFlags;
    status = SecStaticCodeCheckValidity(staticCode, flags, NULL);
    if (status != errSecSuccess) {
        lastError_ << "SecStaticCodeCheckValidity failed: " << status;
        return false;
    }

    status = SecCodeCopySigningInformation(staticCode, kSecCSSigningInformation, &signingDetails);
    if (status != errSecSuccess) {
        lastError_ << "SecCodeCopySigningInformation failed: " << status;
        return false;
    }

    NSArray *certificateChain = [((__bridge NSDictionary*)signingDetails) objectForKey: (__bridge NSString*)kSecCodeInfoCertificates];
    if (certificateChain.count == 0) {
        lastError_ << "certificate chain count is zero";
        return false;
    }

    for (NSUInteger index = 0; index < certificateChain.count; index++) {
        SecCertificateRef certificate = (__bridge SecCertificateRef)([certificateChain objectAtIndex:index]);
        CFStringRef commonName = NULL;
        if ((errSecSuccess == SecCertificateCopyCommonName(certificate, &commonName))) {
            if (NULL != commonName) {
                auto releaseName = wsl::wsScopeGuard([&] {
                    CFRelease(commonName);
                });
                if (CFEqual((CFTypeRef)commonName, (CFTypeRef)@MACOS_CERT_DEVELOPER_ID)) {
                    return true;
                }
            }
        }
    }

    return false;
}
