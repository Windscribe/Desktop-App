#pragma once

#include <QObject>

class TestRouteEventFilter : public QObject
{
    Q_OBJECT

private slots:
    void physicalRouteRefreshes();
    void tunnelRouteIsTunnelOnly();
    void routeWithoutOutputInterfaceRefreshes();
    void multipathThroughTunnelsRefreshes();
    void unknownIndexRefreshes();
    void goneTunnelTrustedOnlyWithinWindow();
    void renamedTunnelRefreshes();
    void mixedDatagramRefreshes();
    void malformedOrTruncatedRefreshes();
    void nonRouteMessageRefreshes();
    void emptyDatagramRefreshes();
    void wakeupRefreshesPerPhysicalDatagram();
    void wakeupTunnelOnlySignalsOnce();
    void wakeupMixedRefreshesAndChecks();
    void wakeupOverflowRefreshes();
    void wakeupStopsOnOtherErrors();
    void wakeupReadsAtMostCap();
    void defaultRouteTrackerComparesWithLastRead();
};
