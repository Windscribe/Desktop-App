#pragma once

#include <QString>
#include "locationid.h"
#include "networkinterface.h"

namespace types {

struct GuiPersistentState
{
    GuiPersistentState() = default;

    bool isFirewallOn = false;
    qint32 windowOffsX = INT_MAX; // deprecated
    qint32 windowOffsY = INT_MAX; // deprecated
    qint32 locationsViewportHeight = 280; // 7 * 40
    bool isFirstLogin = true;
    bool isIgnoreCpuUsageWarnings = false;
    LocationID lastLocation;
    QString lastExternalIp = "---.---.---.---";
    QVector<types::NetworkInterface> networkWhiteList;
    QByteArray appGeometry;

    // added in ver 3
    int preferencesWindowHeight = 0;      // 0 -> not set

    // added in ver 4
    LOCATION_TAB lastLocationTab = LOCATION_TAB_ALL_LOCATIONS;

    // added in ver 5
    bool isIgnoreLocationServicesDisabled = false;

    // added in ver 6
    bool isIgnoreNotificationDisabled = false;

    bool operator==(const GuiPersistentState &other) const
    {
        return other.isFirewallOn == isFirewallOn &&
               other.windowOffsX == windowOffsX &&
               other.windowOffsY == windowOffsY &&
               other.locationsViewportHeight == locationsViewportHeight &&
               other.isFirstLogin == isFirstLogin &&
               other.isIgnoreCpuUsageWarnings == isIgnoreCpuUsageWarnings &&
               other.lastLocation == lastLocation &&
               other.lastExternalIp == lastExternalIp &&
               other.networkWhiteList == networkWhiteList &&
               other.appGeometry == appGeometry &&
               other.preferencesWindowHeight == preferencesWindowHeight &&
               other.lastLocationTab == lastLocationTab &&
               other.isIgnoreLocationServicesDisabled == isIgnoreLocationServicesDisabled &&
               other.isIgnoreNotificationDisabled == isIgnoreNotificationDisabled;
    }

    bool operator!=(const GuiPersistentState &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream& stream, const GuiPersistentState& o)
    {
        stream << versionForSerialization_;
        stream << o.isFirewallOn << o.windowOffsX << o.windowOffsY <<
                  o.isFirstLogin << o.isIgnoreCpuUsageWarnings << o.lastLocation << o.lastExternalIp <<
                  o.networkWhiteList << o.appGeometry << o.preferencesWindowHeight << o.lastLocationTab <<
                  o.isIgnoreLocationServicesDisabled << o.isIgnoreNotificationDisabled << o.locationsViewportHeight;

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

        qint32 countVisibleLocations = 7;
        stream >> o.isFirewallOn >> o.windowOffsX >> o.windowOffsY;
        if (version < 7) {
            stream >> countVisibleLocations;
        }
        stream >> o.isFirstLogin >> o.isIgnoreCpuUsageWarnings >> o.lastLocation >> o.lastExternalIp >> o.networkWhiteList;

        if (version >= 2) {
            stream >> o.appGeometry;
        }

        if (version >= 3) {
            stream >> o.preferencesWindowHeight;
        }

        if (version >= 4) {
            stream >> o.lastLocationTab;
        }

        if (version >= 5) {
            stream >> o.isIgnoreLocationServicesDisabled;
        }

        if (version >= 6) {
            stream >> o.isIgnoreNotificationDisabled;
        }

        if (version >= 7) {
            stream >> o.locationsViewportHeight;
        } else {
            o.locationsViewportHeight = countVisibleLocations * 40;
        }

        return stream;
    }

private:
    static constexpr int versionForSerialization_ = 7;  // should increment the version if the data format is changed
};

} // types namespace
