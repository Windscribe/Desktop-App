#pragma once

#include <QDebug>
#include <QString>
#include <QJsonObject>
#include "enums.h"
#include "sharesecurehotspot.h"
#include "shareproxygateway.h"
#include "splittunneling.h"
#include "backgroundsettings.h"

namespace types {

struct GuiSettings
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kAppSkinProp = "appSkin";
        const QString kBackgroundSettingsProp = "backgroundSettings";
        const QString kIsAutoConnectProp = "isAutoConnect";
        const QString kIsAutoSecureNetworksProp = "isAutoSecureNetworks";
        const QString kIsDockedToTrayProp = "isDockedToTray";
        const QString kIsHideFromDockProp = "isHideFromDock";
        const QString kIsLaunchOnStartupProp = "isLaunchOnStartup";
        const QString kIsMinimizeAndCloseToTrayProp = "isMinimizeAndCloseToTray";
        const QString kIsShowLocationHealthProp = "isShowLocationHealth";
        const QString kIsShowNotificationsProp = "isShowNotifications";
        const QString kIsStartMinimizedProp = "isStartMinimized";
        const QString kLatencyDisplayProp = "latencyDisplay";
        const QString kOrderLocationProp = "orderLocation";
        const QString kShareProxyGatewayProp = "shareProxyGateway";
        const QString kShareSecureHotspotProp = "shareSecureHotspot";
        const QString kSplitTunnelingProp = "splitTunneling";
        const QString kTrayIconColorProp = "trayIconColor";
        const QString kVersionProp = "version";
    };

    JsonInfo jsonInfo;

    GuiSettings() = default;
    GuiSettings(const QJsonObject& json);

    APP_SKIN appSkin = APP_SKIN_ALPHA;
    BackgroundSettings backgroundSettings;
    bool isAutoConnect = false;
    bool isAutoSecureNetworks = true;
    bool isDockedToTray = false;
    bool isHideFromDock = false;
    bool isLaunchOnStartup = true;
    bool isMinimizeAndCloseToTray = false;
    bool isShowLocationHealth = false;
    bool isShowNotifications = false;
    bool isStartMinimized = false;
    LATENCY_DISPLAY_TYPE latencyDisplay = LATENCY_DISPLAY_BARS;
    ORDER_LOCATION_TYPE orderLocation = ORDER_LOCATION_BY_GEOGRAPHY;
    ShareProxyGateway shareProxyGateway;
    ShareSecureHotspot shareSecureHotspot;
    SplitTunneling splitTunneling;
    TRAY_ICON_COLOR trayIconColor = TRAY_ICON_COLOR_WHITE;

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
               other.appSkin == appSkin &&
               other.trayIconColor == trayIconColor;
    }

    bool operator!=(const GuiSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kAppSkinProp] = static_cast<int>(appSkin);
        json[jsonInfo.kBackgroundSettingsProp] = backgroundSettings.toJson();
        json[jsonInfo.kIsAutoConnectProp] = isAutoConnect;
        json[jsonInfo.kIsAutoSecureNetworksProp] = isAutoSecureNetworks;
        json[jsonInfo.kIsDockedToTrayProp] = isDockedToTray;
        json[jsonInfo.kIsHideFromDockProp] = isHideFromDock;
        json[jsonInfo.kIsLaunchOnStartupProp] = isLaunchOnStartup;
        json[jsonInfo.kIsMinimizeAndCloseToTrayProp] = isMinimizeAndCloseToTray;
        json[jsonInfo.kIsShowLocationHealthProp] = isShowLocationHealth;
        json[jsonInfo.kIsShowNotificationsProp] = isShowNotifications;
        json[jsonInfo.kIsStartMinimizedProp] = isStartMinimized;
        json[jsonInfo.kLatencyDisplayProp] = static_cast<int>(latencyDisplay);
        json[jsonInfo.kOrderLocationProp] = static_cast<int>(orderLocation);
        json[jsonInfo.kShareProxyGatewayProp] = shareProxyGateway.toJson();
        json[jsonInfo.kShareSecureHotspotProp] = shareSecureHotspot.toJson();
        json[jsonInfo.kSplitTunnelingProp] = splitTunneling.toJson();
        json[jsonInfo.kTrayIconColorProp] = static_cast<int>(trayIconColor);
        json[jsonInfo.kVersionProp] = static_cast<int>(versionForSerialization_);

        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const GuiSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.isLaunchOnStartup << o.isAutoConnect << o.isHideFromDock << o.isShowNotifications << o.orderLocation << o.latencyDisplay <<
                  o.shareSecureHotspot << o.shareProxyGateway << o.splitTunneling << o.isDockedToTray << o.isMinimizeAndCloseToTray <<
                  o.backgroundSettings << o.isStartMinimized << o.isShowLocationHealth << o.isAutoSecureNetworks << o.appSkin << o.trayIconColor;

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

        if (version >= 3)
        {
            stream >> o.trayIconColor;
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
        dbg << "appSkin:" << APP_SKIN_toString(gs.appSkin) << ";";
        dbg << "trayIconColor:" << gs.trayIconColor << ";}";

        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 3;  // should increment the version if the data format is changed

};

inline GuiSettings::GuiSettings(const QJsonObject &json)
{
    if (json.contains(jsonInfo.kAppSkinProp) && json[jsonInfo.kAppSkinProp].isDouble())
        appSkin = static_cast<APP_SKIN>(json[jsonInfo.kAppSkinProp].toInt());

    if (json.contains(jsonInfo.kBackgroundSettingsProp) && json[jsonInfo.kBackgroundSettingsProp].isObject())
        backgroundSettings = types::BackgroundSettings(json[jsonInfo.kBackgroundSettingsProp].toObject());

    if (json.contains(jsonInfo.kIsAutoConnectProp) && json[jsonInfo.kIsAutoConnectProp].isBool())
        isAutoConnect = json[jsonInfo.kIsAutoConnectProp].toBool();

#if !defined(Q_OS_LINUX)
    if (json.contains(jsonInfo.kIsDockedToTrayProp) && json[jsonInfo.kIsDockedToTrayProp].isBool())
        isDockedToTray = json[jsonInfo.kIsDockedToTrayProp].toBool();
#endif

#if defined(Q_OS_MAC)
    if (json.contains(jsonInfo.kIsHideFromDockProp) && json[jsonInfo.kIsHideFromDockProp].isBool())
        isHideFromDock = json[jsonInfo.kIsHideFromDockProp].toBool();
#endif

    if (json.contains(jsonInfo.kIsLaunchOnStartupProp) && json[jsonInfo.kIsLaunchOnStartupProp].isBool())
        isLaunchOnStartup = json[jsonInfo.kIsLaunchOnStartupProp].toBool();

    if (json.contains(jsonInfo.kIsMinimizeAndCloseToTrayProp) && json[jsonInfo.kIsMinimizeAndCloseToTrayProp].isBool())
        isMinimizeAndCloseToTray = json[jsonInfo.kIsMinimizeAndCloseToTrayProp].toBool();

    if (json.contains(jsonInfo.kIsShowLocationHealthProp) && json[jsonInfo.kIsShowLocationHealthProp].isBool())
        isShowLocationHealth = json[jsonInfo.kIsShowLocationHealthProp].toBool();

    if (json.contains(jsonInfo.kIsShowNotificationsProp) && json[jsonInfo.kIsShowNotificationsProp].isBool())
        isShowNotifications = json[jsonInfo.kIsShowNotificationsProp].toBool();

    if (json.contains(jsonInfo.kIsStartMinimizedProp) && json[jsonInfo.kIsStartMinimizedProp].isBool())
        isStartMinimized = json[jsonInfo.kIsStartMinimizedProp].toBool();

    if (json.contains(jsonInfo.kLatencyDisplayProp) && json[jsonInfo.kLatencyDisplayProp].isDouble())
        latencyDisplay = static_cast<LATENCY_DISPLAY_TYPE>(json[jsonInfo.kLatencyDisplayProp].toInt());

    if (json.contains(jsonInfo.kOrderLocationProp) && json[jsonInfo.kOrderLocationProp].isDouble())
        orderLocation = static_cast<ORDER_LOCATION_TYPE>(json[jsonInfo.kOrderLocationProp].toInt());

#if defined(Q_OS_LINUX)
    if (json.contains(jsonInfo.kTrayIconColorProp) && json[jsonInfo.kTrayIconColorProp].isDouble())
        trayIconColor = static_cast<TRAY_ICON_COLOR>(json[jsonInfo.kTrayIconColorProp].toInt());
#endif

    if (json.contains(jsonInfo.kShareSecureHotspotProp) && json[jsonInfo.kShareSecureHotspotProp].isObject())
        shareSecureHotspot = types::ShareSecureHotspot(json[jsonInfo.kShareSecureHotspotProp].toObject());

    if (json.contains(jsonInfo.kShareProxyGatewayProp) && json[jsonInfo.kShareProxyGatewayProp].isObject())
        shareProxyGateway = types::ShareProxyGateway(json[jsonInfo.kShareProxyGatewayProp].toObject());

    if (json.contains(jsonInfo.kSplitTunnelingProp) && json[jsonInfo.kSplitTunnelingProp].isObject())
        splitTunneling = types::SplitTunneling(json[jsonInfo.kSplitTunnelingProp].toObject());
}

} // types namespace
