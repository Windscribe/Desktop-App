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

    friend QDataStream& operator <<(QDataStream& stream, const GuiPersistentState& o)
    {
        stream << versionForSerialization_;
        stream << o.isFirewallOn << o.windowOffsX << o.windowOffsY << o.countVisibleLocations <<
                  o.isFirstLogin << o.isIgnoreCpuUsageWarnings << o.lastLocation << o.lastExternalIp <<
                  o.networkWhiteList;

        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, GuiPersistentState& o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }

        stream >> o.isFirewallOn >> o.windowOffsX >> o.windowOffsY >> o.countVisibleLocations >>
                  o.isFirstLogin >> o.isIgnoreCpuUsageWarnings >> o.lastLocation >> o.lastExternalIp >> o.networkWhiteList;
        return stream;
    }


private:
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


} // types namespace

#endif // TYPES_GUIPERSISTENTSTATE_H
