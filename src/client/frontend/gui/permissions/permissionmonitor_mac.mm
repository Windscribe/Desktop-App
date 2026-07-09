#include "permissionmonitor_mac.h"

#import <Cocoa/Cocoa.h>
#import <CoreLocation/CoreLocation.h>

PermissionMonitor_mac *g_PermissionMonitor_mac = NULL;

@interface PermissionMonitor : NSObject<CLLocationManagerDelegate>
- (void) locationManagerDidChangeAuthorization:(CLLocationManager *) manager;
@end

@implementation PermissionMonitor
- (void) locationManagerDidChangeAuthorization:(CLLocationManager *) manager;
{
    if (g_PermissionMonitor_mac) {
        g_PermissionMonitor_mac->onLocationPermissionUpdated();
    }
}
@end

PermissionMonitor *g_PermissionMonitor = nil;
CLLocationManager *g_LocationManager = nil;

PermissionMonitor_mac::PermissionMonitor_mac(QObject *parent)
{
    g_PermissionMonitor_mac  = this;
    g_PermissionMonitor = [[PermissionMonitor alloc] init];

    // Hold the location manager in a file-scope variable so it outlives this constructor; a local would
    // be released immediately under ARC and the permission-change callbacks would never fire.
    g_LocationManager = [[CLLocationManager alloc] init];
    g_LocationManager.delegate = g_PermissionMonitor;
}

PermissionMonitor_mac::~PermissionMonitor_mac()
{
    g_PermissionMonitor_mac = nil;
    g_PermissionMonitor = nil;
    g_LocationManager = nil;
}

void PermissionMonitor_mac::onLocationPermissionUpdated()
{
    emit locationPermissionUpdated();
}
