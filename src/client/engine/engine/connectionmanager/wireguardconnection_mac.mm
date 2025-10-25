#include "wireguardconnection_mac.h"

#import <NetworkExtension/NetworkExtension.h>
#import <Foundation/Foundation.h>
#include <QWaitCondition>
#include <QFuture>
#include <QPromise>

#include <sys/sysctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/route.h>

#include "utils/ws_assert.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils_mac.h"
#include "types/enums.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "../splittunnelextension/systemextensions_mac.h"

WireGuardConnection::WireGuardConnection(QObject *parent, Helper *helper)
    : IConnection(parent),
      helper_(helper),
      state_(STATE_DISCONNECTED)
{
    connect(&statisticsTimer_, &QTimer::timeout, this, &WireGuardConnection::onStatisticsTimer);
}

WireGuardConnection::~WireGuardConnection()
{
}

void WireGuardConnection::startConnect(const QString &configPathOrUrl, const QString &ip,
                                       const QString &dnsHostName, const QString &username,
                                       const QString &password, const types::ProxySettings &proxySettings,
                                       const WireGuardConfig *wireGuardConfig,
                                       bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                                       bool isCustomConfig, const QString &overrideDnsIp)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(username);
    Q_UNUSED(password);
    Q_UNUSED(proxySettings);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isCustomConfig);

    static QWaitCondition waitConditionLocal;
    static QMutex mutexLocal;

    // If the system extension is not active, WireGuard will not work.
    if (SystemExtensions_mac::instance()->getSystemExtensionState() != SystemExtensions_mac::Active) {
        emit error(WIREGUARD_SYSTEMEXTENSION_INACTIVE);
        return;
    }


    mutexLocal.lock();

    isConnectingStateReachedAfterStartingConnection_ = false;

    WS_ASSERT(state_ == STATE_DISCONNECTED);

    state_ = STATE_START_CONNECT;
    startConnect_ = QDateTime::currentDateTime();

    WireGuardConfig config = *wireGuardConfig;
    if (!overrideDnsIp.isEmpty()) {
        config.setClientDnsAddress(overrideDnsIp);
    }

    // note: route gateway not used for WireGuard in AdapterGatewayInfo
    adapterGatewayInfo_.clear();
    QStringList address_and_cidr = wireGuardConfig->clientIpAddress().split('/');
    if (address_and_cidr.size() > 1)
    {
        adapterGatewayInfo_.setAdapterIp(address_and_cidr[0]);
    }
    adapterGatewayInfo_.setDnsServers(QStringList() << wireGuardConfig->clientDnsAddress());

    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> *managers, NSError *err) {
        mutexLocal.lock();
        if (err) {
            qCCritical(LOG_WIREGUARD) << "Error loading tunnel managers:" << QString::fromNSString(err.localizedDescription);
            state_ = STATE_DISCONNECTED;
            emit error(WIREGUARD_FAILED_START_MAC);
            waitConditionLocal.wakeAll();
            mutexLocal.unlock();
        } else {

            // Look for existing tunnel
            NETunnelProviderManager *tunnelProviderManager = nil;
            for (NETunnelProviderManager *manager in managers) {
                if ([manager.localizedDescription isEqualToString:kVpnDescription.toNSString()]) {
                    tunnelProviderManager = manager;
                    break;
                }
            }
            if (!tunnelProviderManager) {
                qCInfo(LOG_WIREGUARD) << "Creating new tunnel:" << kVpnDescription;
                tunnelProviderManager = [[NETunnelProviderManager alloc] init];
                tunnelProviderManager.localizedDescription = kVpnDescription.toNSString();
            } else {
                qCInfo(LOG_WIREGUARD) << "Found existing tunnel:" << kVpnDescription;
            }

            tunnelProviderManager.enabled = YES;

            // Create protocol configuration
            NETunnelProviderProtocol *protocol = [[NETunnelProviderProtocol alloc] init];
            //FIXME: move name outside
            protocol.providerBundleIdentifier = @"com.windscribe.client.networkextension";
            protocol.serverAddress = @"127.0.0.1"; // Dummy address

            // Store the full config in provider configuration
            NSDictionary *configDict = @{
                @"WgQuickConfig": config.generateConfigFile().toNSString()
            };
            protocol.providerConfiguration = configDict;

            tunnelProviderManager.protocolConfiguration = protocol;
            // Save the tunnel manager
            [tunnelProviderManager saveToPreferencesWithCompletionHandler:^(NSError *err) {
                if (err) {
                    qCCritical(LOG_WIREGUARD) << "Error saving tunnel manager:" << QString::fromNSString(err.localizedDescription);
                    state_ = STATE_DISCONNECTED;
                    emit error(WIREGUARD_FAILED_START_MAC);
                    waitConditionLocal.wakeAll();
                    mutexLocal.unlock();
                } else {

                    // load preferences again, otherwise Mac bug (https://forums.developer.apple.com/thread/25928)
                    [tunnelProviderManager loadFromPreferencesWithCompletionHandler:^(NSError *err) {
                        if (err) {
                            qCCritical(LOG_WIREGUARD) << "Error loading preferences:" << QString::fromNSString(err.localizedDescription);
                            state_ = STATE_DISCONNECTED;
                            emit error(WIREGUARD_FAILED_START_MAC);
                            waitConditionLocal.wakeAll();
                            mutexLocal.unlock();
                        } else {
                            notificationId_ = [[NSNotificationCenter defaultCenter] addObserverForName: (NSString *)NEVPNStatusDidChangeNotification object: tunnelProviderManager.connection queue: nil usingBlock: ^ (NSNotification *notification)
                            {
                                this->handleNotification(notification);
                            }];

                            qCInfo(LOG_WIREGUARD) << "NEVPNConnection current status:" << connectionStatusToString(tunnelProviderManager.connection.status);

                            NSError *startError;
                            currentSession_ = tunnelProviderManager.connection;
                            NETunnelProviderSession *session = (NETunnelProviderSession *)currentSession_;
                            [session startTunnelWithOptions:nil andReturnError:&startError];
                            if (startError)
                            {
                                qCWarning(LOG_WIREGUARD) << "Error starting WireGuard connection:" << QString::fromNSString(startError.localizedDescription);
                                [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: session];
                                state_ = STATE_DISCONNECTED;
                                emit error(WIREGUARD_FAILED_START_MAC);
                            }
                            waitConditionLocal.wakeAll();
                            mutexLocal.unlock();
                        }
                    }];
                }
            }];
        }
    }];

    waitConditionLocal.wait(&mutexLocal);
    mutexLocal.unlock();
 }

void WireGuardConnection::startDisconnect()
{
    QMutexLocker locker(&mutex_);
    NETunnelProviderSession *session = (NETunnelProviderSession *)currentSession_;

    if (state_ == STATE_DISCONNECTED)
    {
        statisticsTimer_.stop();
        removeVPNConfigurationFromSystemPreferences();
        emit disconnected();
    }
    else if (state_ != STATE_START_DISCONNECTING)
    {
        state_ = STATE_START_DISCONNECTING;
        if (session.status == NEVPNStatusDisconnected && isConnectingStateReachedAfterStartingConnection_)
        {
            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: session];
            state_ = STATE_DISCONNECTED;
            statisticsTimer_.stop();
            removeVPNConfigurationFromSystemPreferences();
            emit disconnected();
        }
        else
        {
            [session stopTunnel];
        }
    }
}

bool WireGuardConnection::isDisconnected() const
{
    QMutexLocker locker(&mutex_);
    return state_ == STATE_DISCONNECTED;
}

void WireGuardConnection::stopWindscribeActiveConnection()
{
    static QWaitCondition waitCondition;
    static QMutex mutex;

    mutex.lock();

    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> *managers, NSError *err) {
        mutex.lock();
        if (err) {
            waitCondition.wakeAll();
            mutex.unlock();
        } else {

            // Look for existing tunnel
            NETunnelProviderManager *tunnelProviderManager = nil;
            for (NETunnelProviderManager *manager in managers) {
                if ([manager.localizedDescription isEqualToString:kVpnDescription.toNSString()]) {
                    tunnelProviderManager = manager;
                    break;
                }
            }
            if (tunnelProviderManager) {
                qCInfo(LOG_WIREGUARD) << "Previous WireGuard connection is active. Stop it.";
                [((NETunnelProviderSession *)tunnelProviderManager.connection) stopTunnel];

            }
            waitCondition.wakeAll();
            mutex.unlock();
        }
    }];

    waitCondition.wait(&mutex);
    mutex.unlock();
}

void WireGuardConnection::handleNotificationImpl()
{
    QMutexLocker locker(&mutex_);

    NETunnelProviderSession *session = (NETunnelProviderSession *)currentSession_;

    qCInfo(LOG_WIREGUARD) << "Connection status changed:" << connectionStatusToString(session.status);
    if (session.status == NEVPNStatusInvalid) {
        [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: session];
        state_ = STATE_DISCONNECTED;
        statisticsTimer_.stop();
        removeVPNConfigurationFromSystemPreferences();
        emit disconnected();
    } else if (session.status == NEVPNStatusDisconnected) {
        if (state_ != STATE_DISCONNECTED) {
            [[NSNotificationCenter defaultCenter] removeObserver: (id)notificationId_ name: (NSString *)NEVPNStatusDidChangeNotification object: session];
            state_ = STATE_DISCONNECTED;
            statisticsTimer_.stop();
            removeVPNConfigurationFromSystemPreferences();
            emit disconnected();
        }
    } else if (session.status == NEVPNStatusConnecting) {
        isConnectingStateReachedAfterStartingConnection_ = true;
    } else if (session.status == NEVPNStatusConnected) {
        state_ = STATE_CONNECTED;

        // Request runtime configuration to obtain the name of the network adapter
        static QWaitCondition waitConditionLocal;
        static QMutex mutexLocal;
        mutexLocal.lock();

        NSData *messageData = [NSData dataWithBytes:(UInt8[]){1} length:1];
        NSError *sendError = nil;
        BOOL success = [session sendProviderMessage:messageData
                                         returnError:&sendError
                                     responseHandler:^(NSData *responseData) {
            if (responseData) {
                NSString *adapterNameString = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
                adapterGatewayInfo_.setAdapterName(QString::fromNSString(adapterNameString));
            } else {
                qCCritical(LOG_WIREGUARD) << "Unable to obtain the name of the VPN adapter from the network extension.";
            }
            waitConditionLocal.wakeAll();
        }];
        if (success) {
            waitConditionLocal.wait(&mutexLocal);
            mutexLocal.unlock();
        } else {
            qCCritical(LOG_WIREGUARD) << "Unable to obtain the name of the VPN adapter from the network extension, sendProviderMessage function returned false";
        }

        emit connected(adapterGatewayInfo_);
        statisticsTimer_.start(STATISTICS_UPDATE_PERIOD);
    } else if (session.status == NEVPNStatusReasserting) {
        emit reconnecting();
    } else if (session.status == NEVPNStatusDisconnecting) {
        /*if (state_ == STATE_START_CONNECT)
        {
            // We've encountered instances on macOS with the 'log show' command taking quite some time to run (> 10s).
            // These debugs will help us detect this edge case, so we don't go off in the weeds looking for other
            // causes of the delay.
            qCDebug(LOG_NETWORK_EXTENSION_MAC) << "NetworkExtensionLog_mac starting log read process";
            NetworkExtensionLog_mac neLog;
            QMap<time_t, QString> logs = neLog.collectLogs(startConnect_);
            qCDebug(LOG_NETWORK_EXTENSION_MAC) << "NetworkExtensionLog_mac log read process finished";
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
        }*/
    }
}

void WireGuardConnection::onStatisticsTimer()
{
    if (!adapterGatewayInfo_.adapterName().isEmpty())
    {
        unsigned int adapterInd = if_nametoindex(adapterGatewayInfo_.adapterName().toStdString().c_str());
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

void WireGuardConnection::handleNotification(void *notification)
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "handleNotificationImpl");
}

QString WireGuardConnection::connectionStatusToString(int status) const
{
    if (status == NEVPNStatusInvalid)
      return "NEVPNStatusInvalid";
    else if (status == NEVPNStatusDisconnected)
      return "NEVPNStatusDisconnected";
    else if (status == NEVPNStatusConnecting)
      return "NEVPNStatusConnecting";
    else if (status == NEVPNStatusConnected)
      return "NEVPNStatusConnected";
    else if (status == NEVPNStatusReasserting)
      return "NEVPNStatusReasserting";
    else if (status == NEVPNStatusDisconnecting)
      return "NEVPNStatusDisconnecting";
    else
      return "Unknown";
}

void WireGuardConnection::removeVPNConfigurationFromSystemPreferences()
{
    auto promise = std::make_shared<QPromise<void>>();
    auto future = promise->future();

    // Remove VPN configuration from system preferences
    [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETunnelProviderManager *> *managers, NSError *err) {
        if (!err) {
            bool foundManager = false;
            for (NETunnelProviderManager *manager in managers) {
                if ([manager.localizedDescription isEqualToString:kVpnDescription.toNSString()]) {
                    foundManager = true;
                    manager.enabled = NO;
                    [manager removeFromPreferencesWithCompletionHandler:^(NSError *removeError) {
                        if (removeError) {
                            qCWarning(LOG_WIREGUARD) << "Error removing VPN configuration:" << QString::fromNSString(removeError.localizedDescription);
                        }
                        promise->finish();
                    }];
                    break;
                }
            }
            if (!foundManager) {
                // If no manager was found, complete the promise
                promise->finish();
            }
        } else {
            qCWarning(LOG_WIREGUARD) << "Error removing VPN configuration:" << QString::fromNSString(err.localizedDescription);
            promise->finish();
        }
    }];

    future.waitForFinished();
}
