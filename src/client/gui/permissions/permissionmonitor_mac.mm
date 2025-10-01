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

PermissionMonitor_mac::PermissionMonitor_mac(QObject *parent)
{
    g_PermissionMonitor_mac  = this;
    g_PermissionMonitor = [[PermissionMonitor alloc] init];

    CLLocationManager *locationManager = [[CLLocationManager alloc] init];
    locationManager.delegate = g_PermissionMonitor;
}

PermissionMonitor_mac::~PermissionMonitor_mac()
{
    [(id)g_PermissionMonitor_mac release];
    g_PermissionMonitor_mac = nil;
    g_PermissionMonitor = NULL;
}

void PermissionMonitor_mac::onLocationPermissionUpdated()
{
    emit locationPermissionUpdated();
}
