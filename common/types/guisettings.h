#ifndef TYPES_GUISETTINGS_H
#define TYPES_GUISETTINGS_H

#include <QString>
#include "enums.h"
#include "sharesecurehotspot.h"
#include "shareproxygateway.h"
#include "splittunneling.h"
#include "backgroundsettings.h"

namespace types {

struct GuiSettings
{
    bool isLaunchOnStartup = true;
    bool isAutoConnect = false;
    bool isHideFromDock = false;
    bool isShowNotifications = false;
    ORDER_LOCATION_TYPE orderLocation = ORDER_LOCATION_BY_GEOGRAPHY;
    LATENCY_DISPLAY_TYPE latencyDisplay = LATENCY_DISPLAY_BARS;
    ShareSecureHotspot shareSecureHotspot;
    ShareProxyGateway shareProxyGateway;
    SplitTunneling splitTunneling;
    bool isDockedToTray = false;
    bool isMinimizeAndCloseToTray = false;
    BackgroundSettings backgroundSettings;
    bool isStartMinimized = false;
    bool isShowLocationHealth = false;

    bool operator==(const GuiSettings &other) const
    {
        return other.isLaunchOnStartup == isLaunchOnStartup &&
               other.isAutoConnect == isAutoConnect &&
               other.isHideFromDock == isHideFromDock &&
               other.isShowNotifications == isShowNotifications &&
               other.orderLocation == orderLocation &&
               other.latencyDisplay == latencyDisplay &&
               other.shareSecureHotspot == shareSecureHotspot &&
               other.shareProxyGateway == shareProxyGateway &&
               other.splitTunneling == splitTunneling &&
               other.isDockedToTray == isDockedToTray &&
               other.isMinimizeAndCloseToTray == isMinimizeAndCloseToTray &&
               other.backgroundSettings == backgroundSettings &&
               other.isStartMinimized == isStartMinimized &&
               other.isShowLocationHealth == isShowLocationHealth;
    }

    bool operator!=(const GuiSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const GuiSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.isLaunchOnStartup << o.isAutoConnect << o.isHideFromDock << o.isShowNotifications << o.orderLocation << o.latencyDisplay <<
                  o.shareSecureHotspot << o.shareProxyGateway << o.splitTunneling << o.isDockedToTray << o.isMinimizeAndCloseToTray <<
                  o.backgroundSettings << o.isStartMinimized << o.isShowLocationHealth;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, GuiSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isLaunchOnStartup >> o.isAutoConnect >> o.isHideFromDock >> o.isShowNotifications >> o.orderLocation >> o.latencyDisplay >>
                  o.shareSecureHotspot >> o.shareProxyGateway >> o.splitTunneling >> o.isDockedToTray >> o.isMinimizeAndCloseToTray >>
                  o.backgroundSettings >> o.isStartMinimized >> o.isShowLocationHealth;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};


} // types namespace

#endif // TYPES_GUISETTINGS_H
