#ifndef TYPES_GUIPERSISTENTSTATE_H
#define TYPES_GUIPERSISTENTSTATE_H

#include <QString>
#include "locationid.h"
#include "networkinterface.h"

namespace types {

struct GuiPersistentState
{
    GuiPersistentState() :
        isFirewallOn(false),
        windowOffsX(INT_MAX),
        windowOffsY(INT_MAX),
        countVisibleLocations(7),
        isFirstLogin(true),
        isIgnoreCpuUsageWarnings(false),
        lastExternalIp("N/A")
    {}

    bool isFirewallOn;
    qint32 windowOffsX;
    qint32 windowOffsY;
    qint32 countVisibleLocations;
    bool isFirstLogin;
    bool isIgnoreCpuUsageWarnings;
    LocationID lastLocation;
    QString lastExternalIp;
    QVector<types::NetworkInterface> networkWhiteList;

    bool isWindowOffsSetled() const
    {
        return windowOffsX != INT_MAX && windowOffsY != INT_MAX;
    }

    bool operator==(const GuiPersistentState &other) const
    {
        return other.isFirewallOn == isFirewallOn &&
               other.windowOffsX == windowOffsX &&
               other.windowOffsY == windowOffsY &&
               other.countVisibleLocations == countVisibleLocations &&
               other.isFirstLogin == isFirstLogin &&
               other.isIgnoreCpuUsageWarnings == isIgnoreCpuUsageWarnings &&
               other.lastLocation == lastLocation &&
               other.lastExternalIp == lastExternalIp &&
               other.networkWhiteList == networkWhiteList;
    }

    bool operator!=(const GuiPersistentState &other) const
    {
        return !(*this == other);
    }
};


} // types namespace

#endif // TYPES_GUIPERSISTENTSTATE_H
