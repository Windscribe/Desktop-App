#import <NetworkExtension/NetworkExtension.h>
#include "splittunnelextensionmanager_mac.h"
#include "types/ipaddress.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/macutils.h"

static NSDictionary *getExtensionOptions(const QString &primaryInterface, const QString &vpnInterface, bool isActive, bool isExclude, const QStringList &appPaths, const QStringList &ips, const QStringList &hostnames)
{
    NSMutableDictionary *options = [[NSMutableDictionary alloc] init];

    NSMutableArray<NSString *> *appPathsArray = [[NSMutableArray alloc] init];
    for (const QString &appPath : appPaths) {
        [appPathsArray addObject:appPath.toNSString()];
    }
    if (!isExclude) {
        // For inclusive tunnels, always add the client itself to the tunnel as with other platforms.
        [appPathsArray addObject:[[NSBundle mainBundle] bundlePath]];
    }
    [options setObject:appPathsArray forKey:@"appPaths"];
    [options setObject:primaryInterface.toNSString() forKey:@"primaryInterface"];
    [options setObject:vpnInterface.toNSString() forKey:@"vpnInterface"];
    [options setObject:(isActive ? @"1" : @"0") forKey:@"isActive"];
    [options setObject:(isExclude ? @"1" : @"0") forKey:@"isExclude"];

    // Family-split the IPs list across two keys: `ips` carries v4 literals, `ipsV6` carries v6.
    // The extension consumes both. Keys are additive, so an extension that only knows about `ips`
    // still gets the v4 entries it expects.
    //
    // Validate each entry by round-tripping through types::IpAddressRange (handles both bare IPs
    // and CIDR forms). detectFamily is a syntactic colon-presence check, so a port-suffixed v4
    // literal like `1.2.3.4:80` lands in the v6 bucket and is silently lost extension-side
    // (lookup keys are inet_ntop output, which never matches `1.2.3.4:80`). The validation here
    // drops any entry that doesn't parse as a real address before it reaches the extension, with
    // a log line so the user can correlate "my split-tunnel rule isn't applying" against the
    // input they pasted.
    NSMutableArray<NSString *> *ipsArray = [[NSMutableArray alloc] init];
    NSMutableArray<NSString *> *ipsV6Array = [[NSMutableArray alloc] init];
    for (const QString &ip : ips) {
        const std::string ipStd = ip.toStdString();
        const types::IpAddressRange range(ipStd);
        if (!range.isValid()) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Dropping invalid split-tunnel IP entry:" << ip;
            continue;
        }
        if (range.isV6()) {
            [ipsV6Array addObject:ip.toNSString()];
        } else {
            [ipsArray addObject:ip.toNSString()];
        }
    }
    [options setObject:ipsArray forKey:@"ips"];
    [options setObject:ipsV6Array forKey:@"ipsV6"];

    NSMutableArray<NSString *> *hostnamesArray = [[NSMutableArray alloc] init];
    for (const QString &hostname : hostnames) {
        [hostnamesArray addObject:hostname.toNSString()];
    }
    [options setObject:hostnamesArray forKey:@"hostnames"];

    [options setObject:(ExtraConfig::instance().getLogSplitTunnelExtension() ? @"1" : @"0") forKey:@"debug"];

    return options;
}

SplitTunnelExtensionManager &SplitTunnelExtensionManager::instance()
{
    static SplitTunnelExtensionManager instance;
    return instance;
}

SplitTunnelExtensionManager::SplitTunnelExtensionManager() : isStarted_(false), isActive_(false), isExclude_(false), appPaths_(), ips_(), hostnames_() {
}

bool SplitTunnelExtensionManager::startExtension(const QString &primaryInterface, const QString &vpnInterface)
{
    if (!isActive_) {
        return false;
    }

    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "Starting split tunnel extension";

    // Get proxy options here.  The interfaces above are references and may no longer be valid by the time the completion handlers run.
    NSDictionary *extensionOptions = getExtensionOptions(primaryInterface, vpnInterface, isActive_, isExclude_, appPaths_, ips_, hostnames_);

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
                if ([protocol.providerBundleIdentifier isEqualToString:@"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID]) {
                    manager = m;
                    break;
                }
            }
        }

        if (!manager) {
            manager = [[NETransparentProxyManager alloc] init];

            NETunnelProviderProtocol *protocol = [[NETunnelProviderProtocol alloc] init];
            protocol.providerBundleIdentifier = @"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;
            protocol.serverAddress = @"127.0.0.1"; // Dummy address

            manager.protocolConfiguration = protocol;
            manager.localizedDescription = @WS_PRODUCT_NAME " Split Tunnel Extension";
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
                } else {
                    isStarted_ = true;
                }
            }];
        }];
    }];

   return true;
}

void SplitTunnelExtensionManager::stopExtension()
{
    if (!isStarted_) {
        return;
    }

    __block dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block id observer = nil;

    [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *error) {
        if (error) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load provider managers:" << QString::fromNSString(error.localizedDescription);
            dispatch_semaphore_signal(semaphore);
            return;
        }

        NETransparentProxyManager *manager = nil;
        for (NETransparentProxyManager *m in managers) {
            if ([m.protocolConfiguration isKindOfClass:[NETunnelProviderProtocol class]]) {
                NETunnelProviderProtocol *protocol = (NETunnelProviderProtocol *)m.protocolConfiguration;
                if ([protocol.providerBundleIdentifier isEqualToString:@"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID]) {
                    manager = m;
                    break;
                }
            }
        }

        if (!manager) {
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Could not find provider manager, nothing to do";
            dispatch_semaphore_signal(semaphore);
            return;
        }

        NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;

        // Set up an observer to monitor the connection status
        observer = [[NSNotificationCenter defaultCenter] addObserverForName:NEVPNStatusDidChangeNotification
                                                          object:session
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:^(NSNotification * _Nonnull notification) {
            NEVPNStatus status = session.status;
            if (status == NEVPNStatusDisconnected || status == NEVPNStatusInvalid) {
                [[NSNotificationCenter defaultCenter] removeObserver:observer];
                observer = nil;
                dispatch_semaphore_signal(semaphore);
            }
        }];

        [session stopTunnel];
    }];

    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC));
    if (dispatch_semaphore_wait(semaphore, timeout) != 0) { // NOLINT
        qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Timeout waiting for extension to stop";
    } else {
        qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel extension disabled";
    }

    if (observer) {
        [[NSNotificationCenter defaultCenter] removeObserver:observer];
    }
    isStarted_ = false;
}

bool SplitTunnelExtensionManager::isActive() const
{
    return isActive_;
}

void SplitTunnelExtensionManager::setSplitTunnelSettings(bool isActive, bool isExclude, const QStringList &appPaths, const QStringList &ips, const QStringList &hostnames)
{
    isActive_ = isActive;
    isExclude_ = isExclude;
    appPaths_ = appPaths;
    ips_ = ips;
    hostnames_ = hostnames;
}
