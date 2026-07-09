#import <NetworkExtension/NetworkExtension.h>
#include "splittunnelextensionmanager_mac.h"
#include "systemextensions_mac.h"
#include "types/ipaddress.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"

// Concurrency model: all session state lives in State and is touched only on the macOS main thread
// (the main dispatch queue and the main NSOperationQueue are mutually serialized there).  The public
// entry points are called from the engine thread and immediately hop onto the main queue; isActive_ is
// the one exception, an atomic settings flag read directly by the engine thread.  Each entry point just
// records the desired state and calls reconcile(), which compares it with the actual session status and
// issues at most one action.  An interface change (connect/reconnect) marks the state dirty and reconcile
// restarts the session so the new interfaces reach the provider as startTunnelWithOptions start options.
// A routing-settings change (apps/IPs/hostnames/mode) to an already-running provider instead goes out as
// a live sendProviderMessage so the provider re-resolves without dropping flows; only if the provider is
// not up does it fall back to the restart path.  The session's status observer and the one-time manager
// setup re-enter reconcile(), so every async event re-derives its action from the current desired state.

struct SplitTunnelExtensionManager::State
{
    NETransparentProxyManager *proxyManager = nil; // cached; nil until setupManager succeeds
    id statusObserver = nil;  // persistent session status observer; drives reconcile()
    bool setupInFlight = false;
    bool wantStarted = false; // desired: a proxy session should be up
    bool startIssued = false; // startTunnel succeeded and the session has not been seen down since;
                              // session.status updates asynchronously, so this guards double-starts
    bool dirty = false;       // settings/interfaces changed since the session last started; restart
    bool isExclude = false;
    QString primaryInterface;
    QString vpnInterface;
    QStringList appPaths;
    QStringList ips;
    QStringList hostnames;
};

// Builds the routing-settings keys (apps/IPs/hostnames/mode) shared by a full start and a live settings
// update.  The interface keys are added only by getExtensionOptions: they can change only on a
// (re)connect and require a tunnel restart, so they never travel in a live update message.
static void addRoutingOptions(NSMutableDictionary *options, bool isExclude, const QStringList &appPaths, const QStringList &ips, const QStringList &hostnames)
{
    NSMutableArray<NSString *> *appPathsArray = [[NSMutableArray alloc] init];
    for (const QString &appPath : appPaths) {
        [appPathsArray addObject:appPath.toNSString()];
    }
    if (!isExclude) {
        // For inclusive tunnels, always add the client itself to the tunnel as with other platforms.
        [appPathsArray addObject:[[NSBundle mainBundle] bundlePath]];
    }
    [options setObject:appPathsArray forKey:@"appPaths"];
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
}

static NSDictionary *getRoutingOptions(bool isExclude, const QStringList &appPaths, const QStringList &ips, const QStringList &hostnames)
{
    NSMutableDictionary *options = [[NSMutableDictionary alloc] init];
    addRoutingOptions(options, isExclude, appPaths, ips, hostnames);
    return options;
}

static NSDictionary *getExtensionOptions(const QString &primaryInterface, const QString &vpnInterface, bool isExclude, const QStringList &appPaths, const QStringList &ips, const QStringList &hostnames)
{
    NSMutableDictionary *options = [[NSMutableDictionary alloc] init];
    addRoutingOptions(options, isExclude, appPaths, ips, hostnames);
    [options setObject:primaryInterface.toNSString() forKey:@"primaryInterface"];
    [options setObject:vpnInterface.toNSString() forKey:@"vpnInterface"];
    return options;
}

static NETransparentProxyManager *findExistingManager(NSArray<NETransparentProxyManager *> *managers)
{
    for (NETransparentProxyManager *m in managers) {
        if ([m.protocolConfiguration isKindOfClass:[NETunnelProviderProtocol class]]) {
            NETunnelProviderProtocol *protocol = (NETunnelProviderProtocol *)m.protocolConfiguration;
            if ([protocol.providerBundleIdentifier isEqualToString:@"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID]) {
                return m;
            }
        }
    }
    return nil;
}

SplitTunnelExtensionManager &SplitTunnelExtensionManager::instance()
{
    // Heap-allocated and intentionally never freed: as a QObject it must not be destroyed at static
    // deinit (which would run on an arbitrary thread and tear down its connections out from under the
    // engine).  Mirrors SystemExtensions_mac's singleton.
    static SplitTunnelExtensionManager *instance = new SplitTunnelExtensionManager();
    return *instance;
}

SplitTunnelExtensionManager::SplitTunnelExtensionManager() : isActive_(false), state_(std::make_unique<State>())
{
}

SplitTunnelExtensionManager::~SplitTunnelExtensionManager() = default;

bool SplitTunnelExtensionManager::isActive() const
{
    return isActive_;
}

void SplitTunnelExtensionManager::setSplitTunnelSettings(bool isActive, bool isExclude, QStringList bundleIds, QStringList ips, QStringList hostnames)
{
    isActive_ = isActive;
    State *state = state_.get();
    dispatch_async(dispatch_get_main_queue(), ^{
        // A restart drops every split flow, so only mark dirty when something actually changed (an
        // unrelated preference change can re-send identical settings).  Disabling is handled by the
        // engine calling stopExtension.
        const bool changed = state->isExclude != isExclude || state->appPaths != bundleIds ||
                             state->ips != ips || state->hostnames != hostnames;
        state->isExclude = isExclude;
        state->appPaths = bundleIds;
        state->ips = ips;
        state->hostnames = hostnames;
        if (isActive && changed) {
            // Update a running provider in place (re-resolves, no flow drop); otherwise restart.
            NETunnelProviderSession *session = (NETunnelProviderSession *)state->proxyManager.connection;
            if (state->startIssued && session && session.status == NEVPNStatusConnected && sendSettingsUpdate()) {
                return;
            }
            state->dirty = true;
            reconcile();
        }
    });
}

// Pushes the current routing lists to the running provider so it re-resolves without a tunnel restart.
// Main queue only.  Returns false if the message could not be sent (the caller then falls back to a
// restart).
bool SplitTunnelExtensionManager::sendSettingsUpdate()
{
    State *state = state_.get();
    NETunnelProviderSession *session = (NETunnelProviderSession *)state->proxyManager.connection;
    if (!session) {
        return false;
    }
    NSDictionary *routing = getRoutingOptions(state->isExclude, state->appPaths, state->ips, state->hostnames);
    NSError *error = nil;
    NSData *message = [NSPropertyListSerialization dataWithPropertyList:routing format:NSPropertyListBinaryFormat_v1_0 options:0 error:&error];
    if (!message) {
        qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to serialize split tunnel settings update:" << QString::fromNSString(error.localizedDescription);
        return false;
    }
    // sendProviderMessage returning YES only means the message was queued, not applied.  The provider
    // acks with a non-empty reply once it has applied the update; an empty reply means it received the
    // message but could not apply it (no session-down to recover from), so restart to re-deliver the
    // settings as start options.  The handler can run on any queue, so hop to main before touching State.
    if (![session sendProviderMessage:message returnError:&error responseHandler:^(NSData *response) {
        if (response.length > 0) {
            return;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            State *state = state_.get();
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Live split tunnel settings update was not applied; restarting";
            state->dirty = true;
            reconcile();
        });
    }]) {
        qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to send split tunnel settings update:" << QString::fromNSString(error.localizedDescription);
        return false;
    }
    qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Sent live split tunnel settings update";
    return true;
}

void SplitTunnelExtensionManager::startExtension(QString primaryInterface, QString vpnInterface)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!isActive_) {
            return;
        }
        State *state = state_.get();
        // On reconnect the interfaces change and the session must restart with them; a recheck with
        // unchanged interfaces is a no-op, so the running session (and its split flows) survives.
        if (state->primaryInterface != primaryInterface || state->vpnInterface != vpnInterface) {
            state->primaryInterface = primaryInterface;
            state->vpnInterface = vpnInterface;
            state->dirty = true;
        }
        state->wantStarted = true;
        reconcile();
    });
}

void SplitTunnelExtensionManager::stopExtension()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        state_->wantStarted = false;
        reconcile();
    });
}

void SplitTunnelExtensionManager::resetManager()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        state_->wantStarted = false;
        state_->startIssued = false;
        reconcile();      // stops a still-live session via the !wantStarted path before we drop the handle
        dropManager();
    });
}

// Runs on the main queue; idempotent.  Compares the desired state with the actual session status and
// issues at most one action; every async completion funnels back here.
void SplitTunnelExtensionManager::reconcile()
{
    State *state = state_.get();
    if (!state->proxyManager) {
        if (state->wantStarted && !state->setupInFlight) {
            setupManager();
        }
        return;
    }

    NETunnelProviderSession *session = (NETunnelProviderSession *)state->proxyManager.connection;
    if (!session) {
        return;
    }
    const NEVPNStatus status = session.status;

    if (!state->wantStarted) {
        // Cleared unconditionally so a stop always resets a start whose session never reported back.
        state->startIssued = false;
        if (status != NEVPNStatusDisconnected && status != NEVPNStatusDisconnecting && status != NEVPNStatusInvalid) {
            [session stopTunnel];
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel extension stop requested";
        }
        return;
    }

    if (!state->startIssued && (status == NEVPNStatusDisconnected || status == NEVPNStatusInvalid)) {
        // Build the options from the current state, so changes that landed while setup or a drain was
        // in progress are included; rapid edits coalesce into this one start.
        NSDictionary *options = getExtensionOptions(state->primaryInterface, state->vpnInterface, state->isExclude,
                                                    state->appPaths, state->ips, state->hostnames);
        state->dirty = false;
        NSError *error = nil;
        if (![session startTunnelWithOptions:options andReturnError:&error]) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to start extension:" << QString::fromNSString(error.localizedDescription);
            // Give up so a failing start is not spammed; the next engine event (connect, settings
            // change, app-activation recheck) re-arms.  Forget the cached manager too: the failure may
            // be the configuration having been disabled or deleted externally, which only
            // setupManager's rebuild path repairs.
            state->wantStarted = false;
            dropManager();
            // An invalid configuration means disabled/removed; leave that to the authoritative
            // system-extension-state path.  Otherwise surface the failure (see the startFailed handler
            // in engine.cpp).  lastKnownState() is touched only on this (main) thread.
            if (status != NEVPNStatusInvalid &&
                SystemExtensions_mac::instance()->lastKnownState() == SystemExtensions_mac::Active) {
                emit startFailed();
            }
        } else {
            state->startIssued = true;
            qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel extension start requested";
        }
        return;
    }

    if (state->dirty && (status == NEVPNStatusConnected || status == NEVPNStatusConnecting)) {
        // Settings or interfaces changed: restart so the new start options deliver them.  The observer
        // re-enters reconcile when the drain reaches Disconnected and the start branch above runs.
        state->startIssued = false;
        [session stopTunnel];
        qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Restarting split tunnel extension to apply new settings";
    }
    // Disconnecting (or waiting for the restart drain): nothing to do; the status observer re-enters
    // here on the next transition.
}

// One-time: find or create the proxy configuration (save+load only when newly created or disabled),
// then cache the manager and park a persistent observer that re-runs reconcile() on every session
// status change -- it starts after a drain completes, restarts after a settings change, and detects
// unexpected session death.
void SplitTunnelExtensionManager::setupManager()
{
    State *state = state_.get();
    state->setupInFlight = true;

    void (^finishSetup)(NETransparentProxyManager *) = ^(NETransparentProxyManager *manager) {
        NETunnelProviderSession *session = (NETunnelProviderSession *)manager.connection;
        if (!session) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to get session";
            state->setupInFlight = false;
            return;
        }
        state->proxyManager = manager;
        state->statusObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NEVPNStatusDidChangeNotification
                                                                                  object:session
                                                                                   queue:[NSOperationQueue mainQueue]
                                                                              usingBlock:^(NSNotification *notification) {
            // A block already enqueued when dropManager removed this observer can still run; ignore
            // anything from a session that is no longer the current one.
            if (session != state->proxyManager.connection) {
                return;
            }
            const NEVPNStatus status = session.status;
            if (state->startIssued && (status == NEVPNStatusDisconnected || status == NEVPNStatusInvalid)) {
                // The status is read live and can be stale: right after a restart issues its start, a
                // queued notification still reads the pre-start Disconnected.  Confirm on a delay -- a
                // live session publishes Connecting well within it; a dead one still reads down.  Do
                // not restart a dead session here (a crashing provider must not loop); a genuine
                // session death is surfaced so the user is alerted and the feature disabled.  A start
                // issued inside the window whose session is genuinely still down is given up along with
                // the dead one.
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    if (!state->startIssued || session != state->proxyManager.connection) {
                        return;
                    }
                    const NEVPNStatus now = session.status;
                    if (now != NEVPNStatusDisconnected && now != NEVPNStatusInvalid) {
                        return;
                    }
                    qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel extension session ended unexpectedly";
                    state->startIssued = false;
                    state->wantStarted = false;
                    if (now == NEVPNStatusInvalid) {
                        // The configuration is gone: the extension was disabled or removed.  Rebuild it
                        // on the next start.
                        dropManager();
                    } else if (SystemExtensions_mac::instance()->lastKnownState() == SystemExtensions_mac::Active) {
                        // Session died with the cache still reading Active (crash or a not-yet-seen
                        // disable): surface the failure (see the startFailed handler in engine.cpp).  A
                        // not-Active cache is owned by the system-extension-state path, so stay silent.
                        // lastKnownState() is touched only on this (main) thread.
                        emit startFailed();
                    }
                });
                return;
            }
            if (status == NEVPNStatusInvalid) {
                // The configuration was deleted externally; rebuild it on the next start.
                dropManager();
            }
            reconcile();
        }];
        state->setupInFlight = false;
        reconcile();
    };

    [NETransparentProxyManager loadAllFromPreferencesWithCompletionHandler:^(NSArray<NETransparentProxyManager *> *managers, NSError *error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (error) {
                qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load provider managers:" << QString::fromNSString(error.localizedDescription);
                state->setupInFlight = false;
                return;
            }
            NETransparentProxyManager *existing = findExistingManager(managers);
            if (existing && existing.enabled) {
                finishSetup(existing);
                return;
            }

            // No configuration yet, or the existing one was disabled in System Settings (startTunnel
            // fails on a disabled configuration and nothing else re-enables it): (re)configure and save.
            NETransparentProxyManager *manager = existing;
            if (!manager) {
                manager = [[NETransparentProxyManager alloc] init];
                NETunnelProviderProtocol *protocol = [[NETunnelProviderProtocol alloc] init];
                protocol.providerBundleIdentifier = @"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;
                protocol.serverAddress = @"127.0.0.1"; // Dummy address
                manager.protocolConfiguration = protocol;
                manager.localizedDescription = @WS_PRODUCT_NAME " Split Tunnel Extension";
                manager.onDemandEnabled = NO;
            }
            manager.enabled = YES;

            [manager saveToPreferencesWithCompletionHandler:^(NSError *error) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (error) {
                        qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to save extension configuration:" << QString::fromNSString(error.localizedDescription);
                        state->setupInFlight = false;
                        return;
                    }
                    [manager loadFromPreferencesWithCompletionHandler:^(NSError *error) {
                        dispatch_async(dispatch_get_main_queue(), ^{
                            if (error) {
                                qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to load configuration for tunnel:" << QString::fromNSString(error.localizedDescription);
                                state->setupInFlight = false;
                                return;
                            }
                            finishSetup(manager);
                        });
                    }];
                });
            }];
        });
    }];
}

// Forgets the cached manager (the configuration was deleted externally); setupManager re-runs on the
// next start.  Runs on the main queue.
void SplitTunnelExtensionManager::dropManager()
{
    State *state = state_.get();
    if (state->statusObserver) {
        [[NSNotificationCenter defaultCenter] removeObserver:state->statusObserver];
        state->statusObserver = nil;
    }
    state->proxyManager = nil;
}
