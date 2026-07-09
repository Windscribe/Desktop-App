#import <AppKit/AppKit.h>
#import <SystemExtensions/SystemExtensions.h>
#include "splittunnelextensionmanager_mac.h"
#include "systemextensions_mac.h"
#include "utils/log/categories.h"

static NSString * const kSplitTunnelBundleId = @"" WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;

// How long to wait for a properties-query callback before abandoning it, so a stuck query cannot hang
// a caller or leak its delegate.  On timeout the unconfirmed fallback is reported, not Inactive.
static const int64_t kPropertiesRequestTimeoutSecs = 3;

// State to report for a query that could not be confirmed (failed or timed out): re-report a confirmed
// Active or PendingUserApproval, degrade anything else to Unknown.  A stale Inactive is never safe to
// re-report -- the engine treats Inactive as confirmed and tears split tunneling down with an error.
static SystemExtensions_mac::SystemExtensionState unconfirmedQueryFallback()
{
    const SystemExtensions_mac::SystemExtensionState last = SystemExtensions_mac::instance()->lastKnownState();
    if (last == SystemExtensions_mac::Active || last == SystemExtensions_mac::PendingUserApproval) {
        return last;
    }
    return SystemExtensions_mac::Unknown;
}

// One delegate per request.  OSSystemExtensionRequest holds its delegate weakly, so each retains
// itself (selfRef) until terminal; results are delivered through onState, so overlapping requests
// cannot interfere.
@interface SystemExtensionRequestDelegate : NSObject <OSSystemExtensionRequestDelegate>
@property (nonatomic) BOOL isPropertiesRequest;
@property (nonatomic) BOOL completed;
@property (nonatomic, copy) void (^onState)(SystemExtensions_mac::SystemExtensionState);
@property (nonatomic, strong) SystemExtensionRequestDelegate *selfRef;
- (void)finishWithState:(SystemExtensions_mac::SystemExtensionState)state;
@end

@implementation SystemExtensionRequestDelegate

// Terminal point: report once (the completed guard drops late/duplicate delivery, e.g. a real callback
// after the watchdog), then release the self-retain after the current callback unwinds.
- (void)finishWithState:(SystemExtensions_mac::SystemExtensionState)state {
    if (self.completed) {
        return;
    }
    self.completed = YES;
    if (self.onState) {
        self.onState(state);
    }
    dispatch_async(dispatch_get_main_queue(), ^{ self.selfRef = nil; });
}

// Properties-query result.  Several entries can share our bundle id (e.g. a staged replacement beside
// the old copy), so prefer enabled over awaiting-approval over disabled; absent means not installed.
- (void)request:(nonnull OSSystemExtensionRequest *)request foundProperties:(nonnull NSArray<OSSystemExtensionProperties *> *)properties {
    BOOL found = NO;
    BOOL anyEnabled = NO;
    BOOL anyAwaitingApproval = NO;
    for (OSSystemExtensionProperties *prop in properties) {
        if ([prop.bundleIdentifier isEqualToString:kSplitTunnelBundleId]) {
            found = YES;
            anyEnabled |= prop.isEnabled;
            anyAwaitingApproval |= prop.isAwaitingUserApproval;
        }
    }

    SystemExtensions_mac::SystemExtensionState state = SystemExtensions_mac::Inactive;
    if (anyEnabled) {
        state = SystemExtensions_mac::Active;
    } else if (anyAwaitingApproval) {
        state = SystemExtensions_mac::PendingUserApproval;
    }
    // Debug level: this fires on every recheck (including each app activation); state changes are
    // logged once in onExtensionStateChanged.
    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension properties: installed=" << (bool)found
        << "enabled=" << (bool)anyEnabled << "awaitingUserApproval=" << (bool)anyAwaitingApproval;
    // A query is a point-in-time snapshot, including PendingUserApproval: macOS sends no notification
    // on approval, so the eventual Active is picked up by the next recheck or an activation's delegate.
    [self finishWithState:state];
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result {
    // A properties query normally resolves in foundProperties: first (making this a no-op via the
    // completed guard); if this ever fires without one, the result is unconfirmed -- never report it
    // as a confirmed Inactive.  An activation request resolves here; its reported value is unused
    // (requestActivation's handler re-queries the real state), so any non-Pending state will do.
    if (self.isPropertiesRequest) {
        [self finishWithState:unconfirmedQueryFallback()];
    } else {
        qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension activation finished with result:" << (int)result;
        [self finishWithState:SystemExtensions_mac::Inactive];
    }
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFailWithError:(nonnull NSError *)error {
    // A failed properties query is unconfirmed; an activation's reported value is unused (see
    // didFinishWithResult).
    qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension request failed:" << QString::fromNSString(error.localizedDescription);
    [self finishWithState:(self.isPropertiesRequest ? unconfirmedQueryFallback()
                                                     : SystemExtensions_mac::Inactive)];
}

- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request {
    // Not terminal: report it, unless the request already finished (e.g. the watchdog fired first).
    if (self.completed) {
        return;
    }
    qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension needs user approval";
    if (self.onState) {
        self.onState(SystemExtensions_mac::PendingUserApproval);
    }
}

- (OSSystemExtensionReplacementAction)request:(nonnull OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(nonnull OSSystemExtensionProperties *)existing
                                withExtension:(nonnull OSSystemExtensionProperties *)extension {
    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension replacement requested, allowing replacement";
    return OSSystemExtensionReplacementActionReplace;
}

@end

// Submits a request with a fresh self-retaining delegate.  Only a properties query gets a watchdog: a
// dropped one must not leak its delegate or wedge the dedup flag.  An activation deliberately has NO
// timeout -- it may wait as long as the user takes in the approval dialog, and timing it out would
// falsely report Inactive mid-approval.
static void submitExtensionRequest(OSSystemExtensionRequest *request, BOOL isPropertiesRequest,
                                   void (^onState)(SystemExtensions_mac::SystemExtensionState)) {
    SystemExtensionRequestDelegate *delegate = [[SystemExtensionRequestDelegate alloc] init];
    delegate.isPropertiesRequest = isPropertiesRequest;
    delegate.onState = onState;
    delegate.selfRef = delegate;
    request.delegate = delegate;
    [[OSSystemExtensionManager sharedManager] submitRequest:request];

    if (!isPropertiesRequest) {
        return;
    }
    __weak SystemExtensionRequestDelegate *weakDelegate = delegate;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, kPropertiesRequestTimeoutSecs * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        SystemExtensionRequestDelegate *strongDelegate = weakDelegate;
        if (strongDelegate && !strongDelegate.completed) {
            qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension properties request timed out";
            [strongDelegate finishWithState:unconfirmedQueryFallback()];
        }
    });
}

SystemExtensions_mac* SystemExtensions_mac::instance_ = nullptr;

SystemExtensions_mac::SystemExtensions_mac() {
    // The extension's OS state can only change behind our back while this app is inactive (the user
    // toggling it in System Settings is in another app), so re-query whenever the app becomes active
    // again.  macOS sends no notification for the toggle itself, and this single trigger keeps the UI
    // warning and the running state fresh with no GUI-side plumbing.
    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *notification) {
        if (SplitTunnelExtensionManager::instance().isActive()) {
            checkSystemExtensionState(false);
        }
    }];
}

SystemExtensions_mac::~SystemExtensions_mac() {
}

void SystemExtensions_mac::onExtensionStateChanged(SystemExtensionState newState) {
    if (newState != lastKnownState_) {
        qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension state changed:" << (int)lastKnownState_ << "->" << (int)newState;
    }
    // Remember the state so a later query that fails or times out can report this instead of a
    // misleading new one.
    lastKnownState_ = newState;
    emit stateChanged(newState);
}

SystemExtensions_mac* SystemExtensions_mac::instance() {
    if (!instance_) {
        instance_ = new SystemExtensions_mac();
    }
    return instance_;
}

void SystemExtensions_mac::requestActivation() {
    OSSystemExtensionRequest *request = [OSSystemExtensionRequest
        activationRequestForExtension:kSplitTunnelBundleId
        queue:dispatch_get_main_queue()];
    if (!request) {
        qCCritical(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to create system extension activation request";
        return;
    }

    qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Submitting system extension activation request";
    submitExtensionRequest(request, NO, ^(SystemExtensions_mac::SystemExtensionState state) {
        if (state == SystemExtensions_mac::PendingUserApproval) {
            SystemExtensions_mac::instance()->onExtensionStateChanged(state);
            return;
        }
        // Terminal activation results are not trusted (see checkSystemExtensionState's doc); re-query
        // the real state instead.  Cannot loop: the re-query never submits another activation.
        SystemExtensions_mac::instance()->checkSystemExtensionState(false);
    });
}

void SystemExtensions_mac::checkSystemExtensionState(bool installIfMissing) {
    // Build and submit on the main queue, where the request and all its callbacks live, so the delegate
    // is only ever touched on one thread (this is invoked from the engine thread).
    dispatch_async(dispatch_get_main_queue(), ^{
        if (isCheckingState_) {
            // A query is already outstanding; let it deliver the state.  Record an install intent so an
            // explicit enable is not swallowed by the dedup.
            if (installIfMissing) {
                pendingInstall_ = true;
            }
            return;
        }
        OSSystemExtensionRequest *request = [OSSystemExtensionRequest
            propertiesRequestForExtension:kSplitTunnelBundleId
            queue:dispatch_get_main_queue()];
        if (!request) {
            qCCritical(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to create system extension properties request";
            queryRetried_ = false;
            onExtensionStateChanged(unconfirmedQueryFallback());
            return;
        }

        isCheckingState_ = true;
        pendingInstall_ = installIfMissing;
        submitExtensionRequest(request, YES, ^(SystemExtensions_mac::SystemExtensionState state) {
            SystemExtensions_mac *inst = SystemExtensions_mac::instance();
            inst->isCheckingState_ = false;
            // A concurrent enable arriving while this query was in flight may have upgraded a passive
            // recheck into an install request; honor that here.
            const bool wantInstall = inst->pendingInstall_;
            inst->pendingInstall_ = false;
            // Retry an unconfirmed (Unknown) query once before reporting it onward; the engine surfaces
            // Unknown as a failure when connected, and one query fault should not turn the feature off.
            if (state == SystemExtensions_mac::Unknown && !inst->queryRetried_) {
                inst->queryRetried_ = true;
                inst->checkSystemExtensionState(wantInstall);
                return;
            }
            inst->queryRetried_ = false;
            const bool stillEnabled = SplitTunnelExtensionManager::instance().isActive();
            // Recovery: when the user is explicitly enabling the feature (wantInstall) and split
            // tunneling is still enabled, submit an activation: it installs/prompts for a confirmed
            // Inactive, a twice-unconfirmed Unknown, or a stuck PendingUserApproval.  The confirmed
            // state is reported regardless, so the engine reacts to the truth while the activation's
            // own callbacks deliver its outcome.
            if (state != SystemExtensions_mac::Active && wantInstall && stillEnabled) {
                qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Split tunnel system extension not usable; requesting activation";
                inst->requestActivation();
            }
            inst->onExtensionStateChanged(state);
        });
    });
}
