#include "reachabilityevents.h"

#include <QScopeGuard>

#import "reachability.h"
#include "utils/log/categories.h"

Reachability *g_Reachability = nil;
ReachAbilityEvents *g_ReachabilityEvents = NULL;

@interface MacReachabilityEvents : NSObject
- (void) receiveNetworkChange:(NSNotification*)notification;
@end

@implementation MacReachabilityEvents : NSObject
- (void) receiveNetworkChange:(NSNotification*)notification
{
    Q_UNUSED(notification);
    if (g_ReachabilityEvents)
    {
        g_ReachabilityEvents->emitNetworkChanged();
    }
}
@end

MacReachabilityEvents *g_MacReachabilityEvents = nil;


void callbackChange(SCDynamicStoreRef store, CFArrayRef changedKeys, void *info)
{
    Q_UNUSED(store);
    Q_UNUSED(changedKeys);
    Q_UNUSED(info);

    if (g_ReachabilityEvents)
    {
        g_ReachabilityEvents->emitNetworkChanged();
    }
}

void ReachAbilityEvents::init()
{
    // use to init the singleton
}

ReachAbilityEvents::ReachAbilityEvents(QObject *parent) : QObject(parent)
{
    g_ReachabilityEvents = this;

    SCDynamicStoreContext context = {0, NULL, NULL, NULL, NULL};
    SCDynamicStoreRef dynStore = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("WindscribeNetworkMonitor"), callbackChange, &context);
    if (dynStore == NULL) {
        qCDebug(LOG_BASIC) << "ReachAbilityEvents - SCDynamicStoreCreate failed";
        return;
    }

    CFArrayRef watchedKeys = NULL;
    auto exitGuard = qScopeGuard([&] {
        if (watchedKeys != NULL) {
            CFRelease(watchedKeys);
        }
        CFRelease(dynStore);
    });

    const CFStringRef keys[3] = { CFSTR("State:/Network/Global/IPv4"), CFSTR("State:/Network/Interface/en0/AirPort") };
    watchedKeys = CFArrayCreate(kCFAllocatorDefault, (const void **)keys, 2, &kCFTypeArrayCallBacks);
    if (watchedKeys == NULL) {
        qCDebug(LOG_BASIC) << "ReachAbilityEvents - CFArrayCreate failed";
        return;
    }

    if (!SCDynamicStoreSetNotificationKeys(dynStore, NULL, watchedKeys)) {
        qCDebug(LOG_BASIC) << "ReachAbilityEvents - SCDynamicStoreSetNotificationKeys failed";
        return;
    }

    CFRunLoopSourceRef rlSrc = SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault, dynStore, 0);
    if (rlSrc != NULL) {
        CFRunLoopAddSource(CFRunLoopGetCurrent(), rlSrc, kCFRunLoopDefaultMode);
        CFRelease(rlSrc);
    } else {
        qCDebug(LOG_BASIC) << "ReachAbilityEvents - SCDynamicStoreCreateRunLoopSource failed";
    }
}

ReachAbilityEvents::~ReachAbilityEvents()
{
    g_Reachability = nil;
    [g_MacReachabilityEvents release];
    g_MacReachabilityEvents = nil;
    g_ReachabilityEvents = NULL;
}

void ReachAbilityEvents::emitNetworkChanged()
{
    emit networkStateChanged();
}
