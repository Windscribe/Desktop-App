
#include "keychain_utils.h"
#import <Foundation/Foundation.h>

namespace KeyChainUtils
{

static const char * trustedAppPaths[] = {
    //"/System/Library/Frameworks/SystemConfiguration.framework/Versions/A/Helpers/SCHelper",
    //"/System/Library/PreferencePanes/Network.prefPane/Contents/XPCServices/com.apple.preference.network.remoteservice.xpc",
    //"/System/Library/CoreServices/SystemUIServer.app",
    //"/usr/sbin/pppd",
    //"/usr/sbin/racoon",
    //"/usr/libexec/configd",
    "/usr/libexec/neagent",
};
    
NSArray *trustedApps()
{
    NSMutableArray *apps = [NSMutableArray array];
    SecTrustedApplicationRef app;
    OSStatus err;
        
    for (int i = 0; i < (sizeof(trustedAppPaths) / sizeof(*trustedAppPaths)); i++)
    {
        err = SecTrustedApplicationCreateFromPath(trustedAppPaths[i], &app);
        if (err == errSecSuccess)
        {
            [apps addObject:(__bridge id)app];
        }
    }
    
    return apps;
}

bool setUsernameAndPassword(const char *label, const char *serviceName, const char *description, const char *username, const char *password)
{
    // first try search existing keychain item
    NSMutableDictionary *searchDictionary = [[NSMutableDictionary alloc] init];
    
    [searchDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
    [searchDictionary setObject:[NSString stringWithUTF8String:serviceName] forKey:(__bridge id)kSecAttrService];
    [searchDictionary setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];
    [searchDictionary setObject:@YES forKey:(__bridge id)kSecReturnRef];
    
    CFTypeRef result = NULL;
    SecItemCopyMatching((__bridge CFDictionaryRef)searchDictionary, &result);
    
    if (result != NULL)
    {
        CFRelease(result);
        
        NSMutableDictionary *newDictionary = [[NSMutableDictionary alloc] init];
        [newDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
        [newDictionary setObject:[NSString stringWithUTF8String:serviceName] forKey:(__bridge id)kSecAttrService];
        [newDictionary setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];
        [newDictionary setObject:@YES forKey:(__bridge id)kSecReturnRef];
        
        NSData *passwordData = [NSData dataWithBytes:(const void *)password length:sizeof(char)*strlen(password)];
        [newDictionary setObject:passwordData forKey:(__bridge id)kSecValueData];
        
        NSData *encodedUsername = [NSData dataWithBytes:(const void *)username length:sizeof(char)*strlen(username)];
        [newDictionary setObject:encodedUsername forKey:(__bridge id)kSecAttrAccount];
        
        OSStatus status = SecItemUpdate((__bridge CFDictionaryRef)searchDictionary, (__bridge CFDictionaryRef)newDictionary);
        if (status != errSecSuccess)
        {
            return false;
        }
    }
    // keychain item not found, create the new
    else
    {
        SecKeychainRef keychain = NULL;
        OSStatus status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem, &keychain);
        if (status != errSecSuccess)
        {
            return false;
        }
        
        status = SecKeychainUnlock(keychain, 0, NULL, FALSE);
        if (status != errSecSuccess)
        {
            CFRelease(keychain);
            return false;
        }
        
        SecKeychainItemRef item = nil;
        SecAccessRef access = nil;
        status = SecAccessCreate(CFSTR("Windscribe ikev2 SecAccess"), (__bridge CFArrayRef)(trustedApps()), &access);
        if(status != noErr)
        {
            CFRelease(keychain);
            return false;
        }
        
        SecKeychainAttribute attrs[] = {
            {kSecLabelItemAttr, (UInt32)strlen(label), (char *)label},
            {kSecGenericItemAttr, (UInt32)strlen(username), (char *)username},
            {kSecServiceItemAttr, (UInt32)strlen(serviceName), (char *)serviceName},
            {kSecDescriptionItemAttr, (UInt32)strlen(description), (char *)description},
        };
        
        SecKeychainAttributeList attributes = {sizeof(attrs) / sizeof(attrs[0]), attrs};
        
        status = SecKeychainItemCreateFromContent(kSecGenericPasswordItemClass, &attributes, (int)strlen(password), password, keychain, access, &item);
        
        if(status != noErr)
        {
            CFRelease(access);
            CFRelease(keychain);
            return false;
        }
        
        CFRelease(access);
        CFRelease(keychain);
        CFRelease(item);
    }
    
    return true;
}
    
}
