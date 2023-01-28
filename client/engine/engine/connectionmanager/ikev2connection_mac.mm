#include "ikev2connection_mac.h"

#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#import <NetworkExtension/NetworkExtension.h>
#import <Foundation/Foundation.h>
#include <QWaitCondition>

#include "utils/logger.h"
#include "utils/macutils.h"
#include "utils/network_utils/network_utils_mac.h"
#include "engine/helper/ihelper.h"

#include <sys/sysctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>

namespace KeyChainUtils
{
    const char szServiceName[] = "aaa.windscribe.com.password.ikev2";
    const char szLabel[] = "aaa.windscribe.com.password.ikev2";
    const char szDescr[] = "Windscribe IKEv2 password";

    static const char * trustedAppPaths[] = {
        "/usr/libexec/neagent"
    };

    NSData *searchKeychainCopyMatching(const char *serviceName)
    {
            NSMutableDictionary *searchDictionary = [[NSMutableDictionary alloc] init];

            [searchDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
            [searchDictionary setObject:[NSString stringWithUTF8String:serviceName] forKey:(__bridge id)kSecAttrService];
            [searchDictionary setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];
            [searchDictionary setObject:@YES forKey:(__bridge id)kSecReturnPersistentRef];

            CFTypeRef result = NULL;
            SecItemCopyMatching((__bridge CFDictionaryRef)searchDictionary, &result);

            return (NSData *)result;
    }


    NSArray *trustedApps()
    {
        NSMutableArray *apps = [NSMutableArray array];
        SecTrustedApplicationRef app;
        OSStatus err;

        // add myself
        err = SecTrustedApplicationCreateFromPath(NULL, &app);
        if (err == errSecSuccess)
        {
            [apps addObject:(__bridge id)app];

        }

        for (size_t i = 0; i < (sizeof(trustedAppPaths) / sizeof(*trustedAppPaths)); i++)
        {
            err = SecTrustedApplicationCreateFromPath(trustedAppPaths[i], &app);
            if (err == errSecSuccess)
            {
                [apps addObject:(__bridge id)app];
            }
        }

        return apps;
    }

    void releaseArrayApps(NSArray *arr)
    {
        for (id object in arr)
        {
            CFRelease(object);
        }
    }

    bool createKeychainItem(const char *username, const char *password)
    {
        SecKeychainRef keychain = NULL;
        OSStatus status = SecKeychainCopyDomainDefault(kSecPreferencesDomainUser, &keychain);
        if (status != errSecSuccess)
        {
            qCDebug(LOG_IKEV2) << "createKeychainItem, SecKeychainCopyDomainDefault return:" << (int)status;
            return false;
        }
        status = SecKeychainUnlock(keychain, 0, NULL, FALSE);  // don't need to check for SecKeychainUnlock errors, as it doesn't affect anything

        SecAccessRef access = nil;
        NSArray *arrApps = trustedApps();
        status = SecAccessCreate(CFSTR("Windscribe IKEv2 SecAccess"), (__bridge CFArrayRef)(arrApps), &access);
        if(status != noErr)
        {
            qCDebug(LOG_IKEV2) << "createKeychainItem, SecAccessCreate return:" << (int)status;
            releaseArrayApps(arrApps);
            CFRelease(keychain);
            return false;
        }

        SecKeychainAttribute attrs[] = {
                    {kSecLabelItemAttr, (UInt32)strlen(szLabel), (char *)szLabel},
                    {kSecGenericItemAttr, (UInt32)strlen(username), (char *)username},
                    {kSecServiceItemAttr, (UInt32)strlen(szServiceName), (char *)szServiceName},
                    {kSecDescriptionItemAttr, (UInt32)strlen(szDescr), (char *)szDescr},
                };

        SecKeychainAttributeList attributes = {sizeof(attrs) / sizeof(attrs[0]), attrs};

        SecKeychainItemRef item = nil;
        status = SecKeychainItemCreateFromContent(kSecGenericPasswordItemClass, &attributes, (int)strlen(password), password, keychain, access, &item);
        if (status != noErr)
        {
            qCDebug(LOG_IKEV2) << "createKeychainItem, SecKeychainItemCreateFromContent return:" << (int)status;
            releaseArrayApps(arrApps);
            CFRelease(access);
            CFRelease(keychain);
            return false;
        }

        CFRelease(item);
        CFRelease(access);
        releaseArrayApps(arrApps);
        CFRelease(keychain);
        return true;
    }

    void removeKeychainItem()
    {
        NSMutableDictionary *searchDictionary = [[NSMutableDictionary alloc] init];

        [searchDictionary setObject:(__bridge id)kSecClassGenericPassword forKey:(__bridge id)kSecClass];
        [searchDictionary setObject:[NSString stringWithUTF8String:szServiceName] forKey:(__bridge id)kSecAttrService];
        [searchDictionary setObject:(__bridge id)kSecMatchLimitAll forKey:(__bridge id)kSecMatchLimit];
        [searchDictionary setObject:@NO forKey:(__bridge id)kSecReturnRef];

        OSStatus status = SecItemDelete((__bridge CFDictionaryRef)searchDictionary);
        if (status != noErr)
        {
            qCDebug(LOG_IKEV2) << "removeKeychainItem, SecItemDelete return:" << (int)status;
        }
        CFRelease(searchDictionary);
    }
}

IKEv2Connection_mac::IKEv2Connection_mac(QObject *parent, IHelper *helper) : IConnection(parent),
    state_(STATE_DISCONNECTED), bConnected_(false), mutex_(QRecursiveMutex()), notificationId_(NULL), isStateConnectingAfterClick_(false), isDisconnectClicked_(false),
    isPrevConnectionStatusInitialized_(false)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);
    networkExtensionLog_.collectNext();
    connect(&statisticsTimer_, SIGNAL(timeout()), SLOT(onStatisticsTimer()));
}

IKEv2Connection_mac::~IKEv2Connection_mac()
{

}

void IKEv2Connection_mac::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password, const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(proxySettings);
    Q_UNUSED(wireGuardConfig);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isAutomaticConnectionMode);
    QMutexLocker locker(&mutex_);

    static QWaitCondition waitConditionLocal;
    static QMutex mutexLocal;

    mutexLocal.lock();

    Q_ASSERT(state_ == STATE_DISCONNECTED);

    isPrevConnectionStatusInitialized_ = false;
    state_ = STATE_START_CONNECT;

    if (!setKeyChain(username, password))
    {
        state_ = STATE_DISCONNECTED;
        emit error(ProtoTypes::ConnectError::IKEV_FAILED_SET_KEYCHAIN_MAC);
        mutexLocal.unlock();
        return;
    }

    NEVPNManager *manager = [NEVPNManager sharedManager];

    NSString *nsUsername = username.toNSString();
    NSString *nsIp = ip.toNSString();
    NSString *nsRemoteId = dnsHostName.toNSString();

    [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
    {
        mutexLocal.lock();

        if (err)
        {
            qCDebug(LOG_IKEV2) << "Load vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
            state_ = STATE_DISCONNECTED;
            emit error(ProtoTypes::ConnectError::IKEV_FAILED_LOAD_PREFERENCES_MAC);
            waitConditionLocal.wakeAll();
            mutexLocal.unlock();
        }
        else
        {
            qCDebug(LOG_IKEV2) << "loadFromPreferences ok";
            NEVPNProtocolIKEv2 *protocol = [[NEVPNProtocolIKEv2 alloc] init];
            protocol.username = nsUsername;
            if (MacUtils::isOsVersionIsCatalina_or_greater())
            {
                protocol.passwordReference = KeyChainUtils::searchKeychainCopyMatching(KeyChainUtils::szServiceName);
            }
            else
            {
                protocol.passwordReference = KeyChainUtils::searchKeychainCopyMatching("Windscribe IKEv2");
            }
            protocol.serverAddress = nsIp;
            protocol.remoteIdentifier = nsRemoteId;
            protocol.useExtendedAuthentication = YES;

            protocol.IKESecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES256GCM;
            protocol.IKESecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup20;
            protocol.IKESecurityAssociationParameters.integrityAlgorithm = NEVPNIKEv2IntegrityAlgorithmSHA256;
            protocol.IKESecurityAssociationParameters.lifetimeMinutes = 1440;

            protocol.childSecurityAssociationParameters.encryptionAlgorithm = NEVPNIKEv2EncryptionAlgorithmAES256GCM;
            protocol.childSecurityAssociationParameters.diffieHellmanGroup = NEVPNIKEv2DiffieHellmanGroup20;
            protocol.childSecurityAssociationParameters.integrityAlgorithm = NEVPNIKEv2IntegrityAlgorithmSHA256;
            protocol.childSecurityAssociationParameters.lifetimeMinutes = 1440;

            //protocol.disconnectOnSleep = YES;
            //[protocol setValue: [NSNumber numberWithBool:YES]  forKey:@"disconnectOnWake"];

            manager.protocolConfiguration = protocol;
            [manager setOnDemandEnabled:NO];
            [manager setLocalizedDescription:@"Windscribe VPN"];
            [manager setEnabled:YES];

            NSString *strProtocol = [NSString stringWithFormat:@"{Protocol: %@", protocol];
            qCDebug(LOG_IKEV2) << QString::fromNSString(strProtocol);

            // do config stuff
            [manager saveToPreferencesWithCompletionHandler:^(NSError *err)
            {
                if (err)
                {
                    qCDebug(LOG_IKEV2) << "Save vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
                    state_ = STATE_DISCONNECTED;
                    emit error(ProtoTypes::ConnectError::IKEV_FAILED_SAVE_PREFERENCES_MAC);
                    waitConditionLocal.wakeAll();
                    mutexLocal.unlock();
                }
                else
                {
                    qCDebug(LOG_IKEV2) << "Save vpn preferences ok";
                     // load  preferences again, otherwise Mac bug (https://forums.developer.apple.com/thread/25928)
                    [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
                    {
                        if (err)
                        {
                            qCDebug(LOG_IKEV2) << "load vpn preferences failed:" << QString::fromNSString(err.localizedDescription);
                            state_ = STATE_DISCONNECTED;
                            emit error(ProtoTypes::ConnectError::IKEV_FAILED_SAVE_PREFERENCES_MAC);
                            waitConditionLocal.wakeAll();
                            mutexLocal.unlock();
                        }
                        else
                        {
                            qCDebug(LOG_IKEV2) << "loadFromPreferences after save ok";
                        }

                        notificationId_ = [[NSNotificationCenter defaultCenter] addObserverForName: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection queue: nil usingBlock: ^ (NSNotification *notification)
                        {
                            this->handleNotification(notification);
                        }];

                        qCDebug(LOG_IKEV2) << "NEVPNConnection current status:" << (int)manager.connection.status;

                        NSError *startError;
                        [manager.connection startVPNTunnelAndReturnError:&startError];
                        if (startError)
                        {
                            qCDebug(LOG_IKEV2) << "Error staring ikev2 connection:" << QString::fromNSString(startError.localizedDescription);
                            state_ = STATE_DISCONNECTED;
                            emit error(ProtoTypes::ConnectError::IKEV_FAILED_START_MAC);
                        }
                        waitConditionLocal.wakeAll();
                        mutexLocal.unlock();
                    }];
                }
            }];
        }
    }];

    waitConditionLocal.wait(&mutexLocal);
    mutexLocal.unlock();
}

void IKEv2Connection_mac::startDisconnect()
{
    QMutexLocker locker(&mutex_);

    if (state_ == STATE_DISCONNECTED)
    {
        statisticsTimer_.stop();
        emit disconnected();
    }
    else if (state_ != STATE_START_DISCONNECTING && state_ != STATE_DISCONNECTING_AUTH_ERROR)
    {
        state_ = STATE_START_DISCONNECTING;
        NEVPNManager *manager = [NEVPNManager sharedManager];
        if (manager.connection.status == NEVPNStatusDisconnected)
        {
            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
            state_ = STATE_DISCONNECTED;
            statisticsTimer_.stop();
            emit disconnected();
        }
        else
        {
            [manager.connection stopVPNTunnel];
        }
    }
}

bool IKEv2Connection_mac::isDisconnected() const
{
    QMutexLocker locker(&mutex_);
    return state_ == STATE_DISCONNECTED;
}

void IKEv2Connection_mac::continueWithUsernameAndPassword(const QString &username, const QString &password)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
    Q_UNUSED(username);
    Q_UNUSED(password);
}

void IKEv2Connection_mac::continueWithPassword(const QString &password)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
    Q_UNUSED(password);
}

void IKEv2Connection_mac::removeIkev2ConnectionFromOS()
{
    KeyChainUtils::removeKeychainItem();
}

void IKEv2Connection_mac::closeWindscribeActiveConnection()
{
    static QWaitCondition waitCondition;
    static QMutex mutex;

    mutex.lock();

    NEVPNManager *manager = [NEVPNManager sharedManager];
    if (manager)
    {
        [manager loadFromPreferencesWithCompletionHandler:^(NSError *err)
        {
            mutex.lock();
            if (!err)
            {
                NEVPNConnection * connection = [manager connection];
                if (connection.status == NEVPNStatusConnected || connection.status == NEVPNStatusConnecting)
                {
                    if ([manager.localizedDescription isEqualToString:@"Windscribe VPN"] == YES)
                    {
                        qCDebug(LOG_IKEV2) << "Previous IKEv2 connection is active. Stop it.";
                        [connection stopVPNTunnel];
                    }
                }
            }
            waitCondition.wakeAll();
            mutex.unlock();
        }];
    }
    waitCondition.wait(&mutex);
    mutex.unlock();
}

void IKEv2Connection_mac::handleNotificationImpl(int status)
{
    QMutexLocker locker(&mutex_);

    NEVPNManager *manager = [NEVPNManager sharedManager];

    if (status == NEVPNStatusInvalid)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusInvalid";
        [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
        state_ = STATE_DISCONNECTED;
        statisticsTimer_.stop();
        emit disconnected();
    }
    else if (status == NEVPNStatusDisconnected)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusDisconnected";

        if (state_ == STATE_DISCONNECTING_ANY_ERROR)
        {
            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
            state_ = STATE_DISCONNECTED;
            emit error(ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT);
        }
        else if (state_ != STATE_DISCONNECTED)
        {
            if (state_ == STATE_DISCONNECTING_AUTH_ERROR)
            {
                emit error(ProtoTypes::ConnectError::AUTH_ERROR);
            }

            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: manager.connection];
            state_ = STATE_DISCONNECTED;
            statisticsTimer_.stop();
            emit disconnected();
        }
    }
    else if (status == NEVPNStatusConnecting)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusConnecting";
    }
    else if (status == NEVPNStatusConnected)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusConnected";
        state_ = STATE_CONNECTED;

        // note: route gateway not used for ikev2 in AdapterGatewayInfo
        AdapterGatewayInfo cai;
        ipsecAdapterName_ = NetworkUtils_mac::lastConnectedNetworkInterfaceName();
        cai.setAdapterName(ipsecAdapterName_);
        cai.setAdapterIp(NetworkUtils_mac::ipAddressByInterfaceName(ipsecAdapterName_));
        cai.setDnsServers(NetworkUtils_mac::getDnsServersForInterface(ipsecAdapterName_));

        emit connected(cai);
        statisticsTimer_.start(STATISTICS_UPDATE_PERIOD);
    }
    else if (status == NEVPNStatusReasserting)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusReasserting";
        emit reconnecting();
    }
    else if (status == NEVPNStatusDisconnecting)
    {
        qCDebug(LOG_IKEV2) << "Connection status changed: NEVPNStatusDisconnecting";

        if (/*isPrevConnectionStatusInitialized_ && prevConnectionStatus_ == NEVPNStatusConnecting &&*/ state_ == STATE_START_CONNECT)
        {
            QMap<time_t, QString> logs = networkExtensionLog_.collectNext();
            for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
            {
                qCDebug(LOG_NETWORK_EXTENSION_MAC) << it.value();
            }
            if (isSocketError(logs))
            {
                state_ = STATE_DISCONNECTING_ANY_ERROR;
            }
            else
            {
                if (isFailedAuthError(logs))
                {
                    state_ = STATE_DISCONNECTING_AUTH_ERROR;
                }
                else
                {
                    state_ = STATE_DISCONNECTING_ANY_ERROR;
                }
            }
        }
    }

    prevConnectionStatus_ = status;
    isPrevConnectionStatusInitialized_ = true;
}

void IKEv2Connection_mac::onStatisticsTimer()
{
    if (!ipsecAdapterName_.isEmpty())
    {
        unsigned int adapterInd = if_nametoindex(ipsecAdapterName_.toStdString().c_str());
        int mib[] = {
                CTL_NET,
                PF_ROUTE,
                0,
                0,
                NET_RT_IFLIST2,
                0
            };
        mib[5] = adapterInd;

        size_t len;
        if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
        {
            return;
        }
        char *buf = (char *)malloc(len);
        if (sysctl(mib, 6, buf, &len, NULL, 0) < 0)
        {
            return;
        }

        char *lim = buf + len;
        char *next = NULL;
        u_int64_t totalibytes = 0;
        u_int64_t totalobytes = 0;
        for (next = buf; next < lim; )
        {
            struct if_msghdr *ifm = (struct if_msghdr *)next;
            next += ifm->ifm_msglen;
            if (ifm->ifm_type == RTM_IFINFO2)
            {
                struct if_msghdr2 *if2m = (struct if_msghdr2 *)ifm;
                totalibytes += if2m->ifm_data.ifi_ibytes;
                totalobytes += if2m->ifm_data.ifi_obytes;
            }
        }
        free(buf);
        if(state_ == STATE_CONNECTED) {
            emit statisticsUpdated(totalibytes, totalobytes, true);
        }
    }

}

bool IKEv2Connection_mac::setKeyChain(const QString &username, const QString &password)
{
    if (MacUtils::isOsVersionIsCatalina_or_greater())
    {
        KeyChainUtils::removeKeychainItem();
        if (!KeyChainUtils::createKeychainItem(username.toStdString().c_str(), password.toStdString().c_str()))
        {
            qCDebug(LOG_IKEV2) << "Can't set username/password in keychain (KeyChainUtils part)";
            return false;
        }
    }
    else
    {
        if (!helper_->setKeychainUsernamePassword(username, password))
        {
            qCDebug(LOG_IKEV2) << "Can't set username/password in keychain (helper part)";
            return false;
        }
    }
    return true;
}

void IKEv2Connection_mac::handleNotification(void *notification)
{
    QMutexLocker locker(&mutex_);
    NSNotification *nsNotification = (NSNotification *)notification;
    NEVPNConnection *connection = nsNotification.object;
    QMetaObject::invokeMethod(this, "handleNotificationImpl", Q_ARG(int, (int)connection.status));
}

bool IKEv2Connection_mac::isFailedAuthError(QMap<time_t, QString> &logs)
{
    for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
    {
        if (it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("IKE", Qt::CaseInsensitive) && it.value().contains("Auth", Qt::CaseInsensitive))
        {
            if (!(it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("IKEv2 socket", Qt::CaseInsensitive)))
            {
                return true;
            }
        }
    }
    return false;
}

bool IKEv2Connection_mac::isSocketError(QMap<time_t, QString> &logs)
{
    for (QMap<time_t, QString>::iterator it = logs.begin(); it != logs.end(); ++it)
    {
        if (it.value().contains("Failed", Qt::CaseInsensitive) && it.value().contains("initialize", Qt::CaseInsensitive) && it.value().contains("socket", Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}


