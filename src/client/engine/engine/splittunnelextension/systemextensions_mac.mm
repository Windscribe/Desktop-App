#import <SystemExtensions/SystemExtensions.h>
#include "systemextensions_mac.h"
#include "utils/log/categories.h"

@interface SystemExtensionDelegate : NSObject <OSSystemExtensionRequestDelegate>
{
    SystemExtensions_mac::SystemExtensionState currentState;
}
- (SystemExtensions_mac::SystemExtensionState)getCurrentState;
@end

@implementation SystemExtensionDelegate

- (id)init {
    self = [super init];
    if (self) {
        currentState = SystemExtensions_mac::Unknown;
    }
    return self;
}

- (SystemExtensions_mac::SystemExtensionState)getCurrentState {
    return currentState;
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result {
    if (result == OSSystemExtensionRequestCompleted) {
        qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension operation completed";
        currentState = SystemExtensions_mac::Active;
    } else {
        qCWarning(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension operation completed with unexpected result:" << (int)result;
        currentState = SystemExtensions_mac::Unknown;
    }
    SystemExtensions_mac::instance()->onExtensionStateChanged(currentState);
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFailWithError:(nonnull NSError *)error {
    qCCritical(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension failed operation:" << QString::fromNSString(error.localizedDescription);
    currentState = SystemExtensions_mac::Inactive;
    SystemExtensions_mac::instance()->onExtensionStateChanged(currentState);
}

- (OSSystemExtensionReplacementAction)request:(nonnull OSSystemExtensionRequest *)request
                     actionForReplacingExtension:(nonnull OSSystemExtensionProperties *)existing
                                   withExtension:(nonnull OSSystemExtensionProperties *)extension {
    qCDebug(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension replacement requested, allowing replacement";
    return OSSystemExtensionReplacementActionReplace;
}

- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request {
    qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "System extension needs user approval";
    currentState = SystemExtensions_mac::PendingUserApproval;
    SystemExtensions_mac::instance()->onExtensionStateChanged(currentState);
}

@end

namespace {
    SystemExtensionDelegate* currentDelegate = nil;
}

SystemExtensions_mac* SystemExtensions_mac::instance_ = nullptr;

SystemExtensions_mac::SystemExtensions_mac() {
}

SystemExtensions_mac::~SystemExtensions_mac() {
}

void SystemExtensions_mac::onExtensionStateChanged(SystemExtensionState newState) {
    emit stateChanged(newState);
}

SystemExtensions_mac* SystemExtensions_mac::instance() {
    if (!instance_) {
        instance_ = new SystemExtensions_mac();
    }
    return instance_;
}

bool SystemExtensions_mac::setAppProxySystemExtensionActive(bool isActive) {
    if (isActive) {
        OSSystemExtensionRequest *request = [OSSystemExtensionRequest
            activationRequestForExtension:@"com.windscribe.client.splittunnelextension"
            queue:dispatch_get_main_queue()];

        if (!request) {
            qCCritical(LOG_SPLIT_TUNNEL_EXTENSION) << "Failed to create system extension request";
            return false;
        }

        if (!currentDelegate) {
            currentDelegate = [[SystemExtensionDelegate alloc] init];
        }
        request.delegate = currentDelegate;
        qCInfo(LOG_SPLIT_TUNNEL_EXTENSION) << "Submitting system extension activation request";
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        // Currently we do nothing here, because deactivating the system extension requires an admin approval dialog,
        // and repeatedly activating/deactivating this way is bad UX.
    }

    return true;
}

SystemExtensions_mac::SystemExtensionState SystemExtensions_mac::getSystemExtensionState() {
    // If we have an active delegate, use its state
    if (currentDelegate) {
        return [currentDelegate getCurrentState];
    }
    return SystemExtensionState::Unknown;
}
