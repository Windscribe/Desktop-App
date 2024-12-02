#include "autodetectproxy_mac.h"
#include "pmachelpers.h"
#include "utils/log/categories.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>

// Ignore deprecated warnings for this file:
// - SecKeychainFindInternetPassword
// - SecKeychainItemFreeContent
// There does not seem to be any replacement for them
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

types::ProxySettings AutoDetectProxy_mac::detect(bool &bSuccessfully)
{
    bSuccessfully = true;
    types::ProxySettings proxySettings;
    proxySettings.setOption(PROXY_OPTION_NONE);

    CFDictionaryRef proxiesDict = CFNetworkCopySystemProxySettings();
    if (proxiesDict)
    {
        QVariantMap proxies = pMacHelpers::toQVariantMap(proxiesDict);

        if (proxies.contains("SOCKSEnable") && proxies["SOCKSEnable"].toLongLong() != 0)
        {
            if (proxies.contains("SOCKSProxy") && proxies.contains("SOCKSPort"))
            {
                proxySettings.setOption(PROXY_OPTION_SOCKS);
                QString hostname = proxies["SOCKSProxy"].toString();
                uint port = proxies["SOCKSPort"].toLongLong();
                proxySettings.setAddress(hostname);
                proxySettings.setPort(port);
                if (proxies.contains("SOCKSUser"))
                {
                    QString username = proxies["SOCKSUser"].toString();
                    proxySettings.setUsername(username);

                    UInt32 passwordLength = 0;
                    void *passwordBuffer = NULL;

                    OSStatus err = SecKeychainFindInternetPassword(
                                NULL,                       // keychain, NULL == default one
                                (UInt32) hostname.length(), hostname.toUtf8().data(), // serverName
                                0, NULL,                    // securityDomain
                                username.length(), username.toUtf8().data(),   // accountName
                                0, NULL,                    // path
                                (UInt16) port,              // port
                                kSecProtocolTypeSOCKS,      // protocol
                                kSecAuthenticationTypeAny,  // authType
                                &passwordLength, &passwordBuffer,   // password buffer
                                NULL                                // keychain item
                        );

                    if (err)
                    {
                        CFStringRef errorStr = SecCopyErrorMessageString(err, NULL);
                        qCCritical(LOG_BASIC) << "MacAutoDetectProxy::detect() -> " << QString::fromCFString(errorStr);
                        CFRelease(errorStr);
                        bSuccessfully = false;
                    }
                    else
                    {
                        qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected SOCKS Proxy with username and password";
                        QString password = QString::fromUtf8((const char *)passwordBuffer, passwordLength);
                        proxySettings.setPassword(password);
                        SecKeychainItemFreeContent(NULL, passwordBuffer);
                    }
                }
                else
                {
                    qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected SOCKS Proxy without username and password";
                }
            }
        }
        else if (proxies.contains("HTTPSEnable") && proxies["HTTPSEnable"].toLongLong() != 0)
        {
            if (proxies.contains("HTTPSProxy") && proxies.contains("HTTPSPort"))
            {
                proxySettings.setOption(PROXY_OPTION_HTTP);
                QString hostname = proxies["HTTPSProxy"].toString();
                uint port = proxies["HTTPSPort"].toLongLong();
                proxySettings.setAddress(hostname);
                proxySettings.setPort(port);
                if (proxies.contains("HTTPSUser"))
                {
                    QString username = proxies["HTTPSUser"].toString();
                    proxySettings.setUsername(username);

                    UInt32 passwordLength = 0;
                    void *passwordBuffer = NULL;

                    OSStatus err = SecKeychainFindInternetPassword(
                                NULL,                       // keychain, NULL == default one
                                (UInt32) hostname.length(), hostname.toUtf8().data(), // serverName
                                0, NULL,                    // securityDomain
                                username.length(), username.toUtf8().data(),   // accountName
                                0, NULL,                    // path
                                (UInt16) port,              // port
                                kSecProtocolTypeHTTPSProxy, // protocol
                                kSecAuthenticationTypeAny,  // authType
                                &passwordLength, &passwordBuffer,   // password buffer
                                NULL                                // keychain item
                        );

                    if (err)
                    {
                        CFStringRef errorStr = SecCopyErrorMessageString(err, NULL);
                        qCCritical(LOG_BASIC) << "MacAutoDetectProxy::detect() -> " << QString::fromCFString(errorStr);
                        CFRelease(errorStr);
                        bSuccessfully = false;
                    }
                    else
                    {
                        qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected HTTPS Proxy with username and password";
                        QString password = QString::fromUtf8((const char *)passwordBuffer, passwordLength);
                        proxySettings.setPassword(password);
                        SecKeychainItemFreeContent(NULL, passwordBuffer);
                    }
                }
                else
                {
                    qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected HTTPS Proxy without username and password";
                }
            }
        }
        else if (proxies.contains("HTTPEnable") && proxies["HTTPEnable"].toLongLong() != 0)
        {
            if (proxies.contains("HTTPProxy") && proxies.contains("HTTPPort"))
            {
                proxySettings.setOption(PROXY_OPTION_HTTP);
                QString hostname = proxies["HTTPProxy"].toString();
                uint port = proxies["HTTPPort"].toLongLong();
                proxySettings.setAddress(hostname);
                proxySettings.setPort(port);
                if (proxies.contains("HTTPUser"))
                {
                    QString username = proxies["HTTPUser"].toString();
                    proxySettings.setUsername(username);

                    UInt32 passwordLength = 0;
                    void *passwordBuffer = NULL;

                    OSStatus err = SecKeychainFindInternetPassword(
                                NULL,                       // keychain, NULL == default one
                                (UInt32) hostname.length(), hostname.toUtf8().data(), // serverName
                                0, NULL,                    // securityDomain
                                username.length(), username.toUtf8().data(),   // accountName
                                0, NULL,                    // path
                                (UInt16) port,              // port
                                kSecProtocolTypeHTTPProxy, // protocol
                                kSecAuthenticationTypeAny,  // authType
                                &passwordLength, &passwordBuffer,   // password buffer
                                NULL                                // keychain item
                        );

                    if (err)
                    {
                        CFStringRef errorStr = SecCopyErrorMessageString(err, NULL);
                        qCCritical(LOG_BASIC) << "MacAutoDetectProxy::detect() -> " << QString::fromCFString(errorStr);
                        CFRelease(errorStr);
                        bSuccessfully = false;
                    }
                    else
                    {
                        qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected HTTP Proxy with username and password";
                        QString password = QString::fromUtf8((const char *)passwordBuffer, passwordLength);
                        proxySettings.setPassword(password);
                        SecKeychainItemFreeContent(NULL, passwordBuffer);
                    }
                }
                else
                {
                    qCInfo(LOG_BASIC) << "MacAutoDetectProxy::detect() -> detected HTTP Proxy without username and password";
                }
            }
        }

        CFRelease(proxiesDict);
    }

    return proxySettings;
}
