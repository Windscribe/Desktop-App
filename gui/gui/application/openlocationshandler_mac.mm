#include "openlocationshandler_mac.h"

#import <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include "utils/macutils.h"

OpenLocationsHandler_mac * g_openLocationsHandler_mac = NULL;

@interface MacOpenLocationsHandler : NSObject
- (void) receiveOpenLocations:(NSNotification*) note ;
- (void) registerListener;
@end

@implementation MacOpenLocationsHandler : NSObject
- (void)receiveOpenLocations:(NSNotification *) note
{
    if (g_openLocationsHandler_mac)
    {
        g_openLocationsHandler_mac->emitReceivedOpenLocationsMessage();
    }
}

- (void) registerListener
{
    NSDistributedNotificationCenter *center = [NSDistributedNotificationCenter defaultCenter];
    [center addObserver:self
            selector:@selector(receiveOpenLocations:)
            name:@"WindscribeGuiOpenLocations"
            object:nil];
}
@end

MacOpenLocationsHandler * g_macOpenLocationsHandler = nil;

OpenLocationsHandler_mac::OpenLocationsHandler_mac(QObject *parent)
{
    g_openLocationsHandler_mac = this;
    g_macOpenLocationsHandler = [MacOpenLocationsHandler alloc];
    [g_macOpenLocationsHandler registerListener];
}

void OpenLocationsHandler_mac::emitReceivedOpenLocationsMessage()
{
    emit receivedOpenLocationsMessage();
}

void OpenLocationsHandler_mac::unsuspendObservers()
{
    NSDistributedNotificationCenter *center = [NSDistributedNotificationCenter defaultCenter];
    [center setSuspended:NO];
}
