#include "sleepevents_mac.h"
#import <Cocoa/Cocoa.h>

SleepEvents_mac *g_SleepEvents = NULL;

@interface MacSleepEvents : NSObject
- (void) receiveSleepNote:(NSNotification*)notification;
@end

@implementation MacSleepEvents : NSObject
- (void) receiveSleepNote:(NSNotification*)notification
{
    Q_UNUSED(notification);
    if (g_SleepEvents)
    {
        g_SleepEvents->gotoSleep();
    }
}
- (void) receiveWakeNote:(NSNotification*)notification
{
    Q_UNUSED(notification);
    if (g_SleepEvents)
    {
        g_SleepEvents->gotoWake();
    }
}
@end

MacSleepEvents *g_MacSleepEvents = nil;

SleepEvents_mac::SleepEvents_mac(QObject *parent) : ISleepEvents(parent)
{
    g_SleepEvents = this;
    g_MacSleepEvents = [[MacSleepEvents alloc] init];
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: g_MacSleepEvents
                selector: @selector(receiveSleepNote:)
                name: NSWorkspaceWillSleepNotification object: NULL];

    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: g_MacSleepEvents
                selector: @selector(receiveWakeNote:)
                name: NSWorkspaceDidWakeNotification object: NULL];
}

SleepEvents_mac::~SleepEvents_mac()
{
    [g_MacSleepEvents release];
    g_MacSleepEvents = nil;
    g_SleepEvents = NULL;
}

void SleepEvents_mac::emitGotoSleep()
{
    emit gotoSleep();
}

void SleepEvents_mac::emitGotoWake()
{
    emit gotoWake();
}
