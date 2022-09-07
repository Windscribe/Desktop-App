#ifndef TYPES_GUISETTINGS_H
#define TYPES_GUISETTINGS_H

#include <QDebug>
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
    bool isAutoSecureNetworks = true;
    APP_SKIN appSkin = APP_SKIN_ALPHA;

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
               other.isShowLocationHealth == isShowLocationHealth &&
               other.isAutoSecureNetworks == isAutoSecureNetworks &&
               other.appSkin == appSkin;
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
                  o.backgroundSettings << o.isStartMinimized << o.isShowLocationHealth << o.isAutoSecureNetworks << o.appSkin;
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
                  o.backgroundSettings >> o.isStartMinimized >> o.isShowLocationHealth >> o.isAutoSecureNetworks;
        if (version >= 2)
        {
            stream >> o.appSkin;
        }
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const GuiSettings &gs)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isLaunchOnStartup:" << gs.isLaunchOnStartup << "; ";
        dbg << "isAutoConnect:" << gs.isAutoConnect << "; ";
        dbg << "isHideFromDock:" << gs.isHideFromDock << "; ";
        dbg << "isShowNotifications:" << gs.isShowNotifications << "; ";
        dbg << "orderLocation:" << ORDER_LOCATION_TYPE_toString(gs.orderLocation) << "; ";
        dbg << "latencyDisplay:" << LATENCY_DISPLAY_TYPE_toString(gs.latencyDisplay) << "; ";
        dbg << "shareSecureHotspot:" << gs.shareSecureHotspot << "; ";
        dbg << "shareProxyGateway:" << gs.shareProxyGateway << "; ";
        dbg << "splitTunneling:" << gs.splitTunneling << "; ";
        dbg << "isDockedToTray:" << gs.isDockedToTray << "; ";
        dbg << "isMinimizeAndCloseToTray:" << gs.isMinimizeAndCloseToTray << "; ";
        dbg << "backgroundSettings:" << gs.backgroundSettings << "; ";
        dbg << "isStartMinimized:" << gs.isStartMinimized << "; ";
        dbg << "isShowLocationHealth:" << gs.isShowLocationHealth << "; ";
        dbg << "isAutoSecureNetworks:" << gs.isAutoSecureNetworks << "; ";
        dbg << "appSkin:" << APP_SKIN_toString(gs.appSkin) << "}";

        return dbg;

    }

private:
    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed

};


} // types namespace

#endif // TYPES_GUISETTINGS_H
