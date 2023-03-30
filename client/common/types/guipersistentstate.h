#ifndef TYPES_GUIPERSISTENTSTATE_H
#define TYPES_GUIPERSISTENTSTATE_H

#include <QString>
#include "locationid.h"
#include "networkinterface.h"

namespace types {

struct GuiPersistentState
{
    GuiPersistentState() {};

    bool isFirewallOn = false;
    qint32 windowOffsX = INT_MAX; // deprecated
    qint32 windowOffsY = INT_MAX; // deprecated
    qint32 countVisibleLocations = 7;
    bool isFirstLogin = true;
    bool isIgnoreCpuUsageWarnings = false;
    LocationID lastLocation;
    QString lastExternalIp = "N/A";
    QVector<types::NetworkInterface> networkWhiteList;
    QByteArray appGeometry;

    // added in ver 3
    int preferencesWindowHeight = 0;      // 0 -> not set

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
               other.networkWhiteList == networkWhiteList &&
               other.appGeometry == appGeometry;
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
                  o.networkWhiteList << o.appGeometry << o.preferencesWindowHeight;

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
                  o.isFirstLogin >> o.isIgnoreCpuUsageWarnings >> o.lastLocation >> o.lastExternalIp >>
                  o.networkWhiteList;

        if (version >= 2) {
            stream >> o.appGeometry;
        }

        if (version >= 3) {
            stream >> o.preferencesWindowHeight;
        }

        return stream;
    }

private:
    static constexpr int versionForSerialization_ = 3;  // should increment the version if the data format is changed
};


} // types namespace

#endif // TYPES_GUIPERSISTENTSTATE_H
