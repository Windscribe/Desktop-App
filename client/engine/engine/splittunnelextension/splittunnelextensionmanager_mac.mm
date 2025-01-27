#import <NetworkExtension/NetworkExtension.h>
#include "splittunnelextensionmanager_mac.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/macutils.h"

static NSDictionary *getExtensionOptions(const QString &primaryInterface, const QString &vpnInterface, bool isActive, bool isExclude, const QStringList &appPaths)
{
    NSMutableDictionary *options = [[NSMutableDictionary alloc] init];

    NSMutableArray<NSString *> *signingIdArray = [[NSMutableArray alloc] init];
    for (const QString &appPath : appPaths) {
        QString signingId = MacUtils::getSigningIdentifier(appPath);
        if (!signingId.isEmpty()) {
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Adding app signing ID:" << signingId;
            [signingIdArray addObject:signingId.toNSString()];
        }
    }
    [options setObject:signingIdArray forKey:@"bundleIds"];
    [options setObject:primaryInterface.toNSString() forKey:@"primaryInterface"];
    [options setObject:vpnInterface.toNSString() forKey:@"vpnInterface"];
    [options setObject:(isActive ? @"1" : @"0") forKey:@"isActive"];
    [options setObject:(isExclude ? @"1" : @"0") forKey:@"isExclude"];
    [options setObject:(ExtraConfig::instance().getLogSplitTunnelExtension() ? @"1" : @"0") forKey:@"debug"];

    return options;
}

SplitTunnelExtensionManager &SplitTunnelExtensionManager::instance()
{
    static SplitTunnelExtensionManager instance;
    return instance;
}

SplitTunnelExtensionManager::SplitTunnelExtensionManager() : isActive_(false), isExclude_(false), appPaths_() {
}

bool SplitTunnelExtensionManager::startExtension(const QString &primaryInterface, const QString &vpnInterface)
{
    if (!isActive_) {
        return false;
    }

    if (appPaths_.isEmpty()) {
        return false;
    }

    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "Starting split tunnel extension";

    // Get proxy options here.  The interfaces above are references and may no longer be valid by the time the completion handlers run.
    NSDictionary *extensionOptions = getExtensionOptions(primaryInterface, vpnInterface, isActive_, isExclude_, appPaths_);

    [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *error) {
        if (error) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load provider managers:" << QString::fromNSString(error.localizedDescription);
            return;
        }

        // Find existing manager or create new one
        NETransparentProxyManager *manager = nil;
        for (NETransparentProxyManager *m in managers) {
            if ([m.protocolConfiguration isKindOfClass:[NETunnelProviderProtocol class]]) {
                NETunnelProviderProtocol *protocol = (NETunnelProviderProtocol *)m.protocolConfiguration;
                if ([protocol.providerBundleIdentifier isEqualToString:@"com.windscribe.client.splittunnelextension"]) {
                    manager = m;
                    break;
                }
            }
        }

        if (!manager) {
            manager = [[NETransparentProxyManager alloc] init];

            NETunnelProviderProtocol *protocol = [[NETunnelProviderProtocol alloc] init];
            protocol.providerBundleIdentifier = @"com.windscribe.client.splittunnelextension";
            protocol.serverAddress = @"127.0.0.1"; // Dummy address

            manager.protocolConfiguration = protocol;
            manager.localizedDescription = @"Windscribe Split Tunnel Extension";
            manager.onDemandEnabled = NO;
            manager.enabled = YES;
        }

        [manager saveToPreferencesWithCompletionHandler:^(NSError *error) {
            if (error) {
                qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to save extension configuration:" << QString::fromNSString(error.localizedDescription);
                return;
            }
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Configuration saved.  Loading...";

            [manager loadFromPreferencesWithCompletionHandler:^(NSError *error) {
                if (error) {
                    qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load configuration for tunnel:" << QString::fromNSString(error.localizedDescription);
                    return;
                }
                qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Configuration loaded.  Starting extension...";

                NSError *ret = nil;
                NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;
                if (!session) {
                    qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to get session";
                    return;
                }

                [session startTunnelWithOptions:extensionOptions andReturnError:&ret];
                if (ret) {
                    qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to start extension:" << QString::fromNSString(ret.localizedDescription);
                }
            }];
        }];
    }];

   return true;
}

void SplitTunnelExtensionManager::stopExtension()
{
    [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *error) {
        if (error) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load provider managers:" << QString::fromNSString(error.localizedDescription);
            return;
        }

        NETransparentProxyManager *manager = nil;
        for (NETransparentProxyManager *m in managers) {
            if ([m.protocolConfiguration isKindOfClass:[NETunnelProviderProtocol class]]) {
                NETunnelProviderProtocol *protocol = (NETunnelProviderProtocol *)m.protocolConfiguration;
                if ([protocol.providerBundleIdentifier isEqualToString:@"com.windscribe.client.splittunnelextension"]) {
                    manager = m;
                    break;
                }
            }
        }

        if (!manager) {
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Could not find provider manager, nothing to do";
            return;
        }

        NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;
        [session stopTunnel];

        qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel extension disabled";
    }];
}

bool SplitTunnelExtensionManager::isActive() const
{
    return isActive_;
}

void SplitTunnelExtensionManager::setSplitTunnelSettings(bool isActive, bool isExclude, const QStringList &appPaths)
{
    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "Setting split tunnel settings:" << isActive << isExclude << appPaths;
    isActive_ = isActive;
    isExclude_ = isExclude;
    appPaths_ = appPaths;
}
