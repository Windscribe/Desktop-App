#include "reachabilityevents.h"
#import "reachability.h"

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

    SCDynamicStoreRef dynStore;

    SCDynamicStoreContext context = {0, NULL, NULL, NULL, NULL};

    dynStore = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("WindscribeNetworkMonitor"), callbackChange, &context);
    const CFStringRef keys[3] = { CFSTR("State:/Network/Global/IPv4"), CFSTR("State:/Network/Interface/en0/AirPort") };
    CFArrayRef watchedKeys = CFArrayCreate(kCFAllocatorDefault,
                                          (const void **)keys,
                                          2,
                                          &kCFTypeArrayCallBacks);

     if (!SCDynamicStoreSetNotificationKeys(dynStore, NULL, watchedKeys))
     {
        CFRelease(watchedKeys);
        CFRelease(dynStore);
        dynStore = NULL;
        return;
     }
     CFRelease(watchedKeys);

     CFRunLoopSourceRef rlSrc = SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault, dynStore, 0);
     CFRunLoopAddSource(CFRunLoopGetCurrent(), rlSrc, kCFRunLoopDefaultMode);
     CFRelease(rlSrc);
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
