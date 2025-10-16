#pragma once

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QString>

#include "backgroundsettings.h"
#include "enums.h"
#include "sharesecurehotspot.h"
#include "shareproxygateway.h"
#include "splittunneling.h"
#include "soundsettings.h"

namespace types {

struct GuiSettings
{

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
    ORDER_LOCATION_TYPE orderLocation = ORDER_LOCATION_BY_GEOGRAPHY;
    ShareProxyGateway shareProxyGateway;
    ShareSecureHotspot shareSecureHotspot;
    SplitTunneling splitTunneling;
    TRAY_ICON_COLOR trayIconColor = TRAY_ICON_COLOR_default();
    MULTI_DESKTOP_BEHAVIOR multiDesktopBehavior = MULTI_DESKTOP_AUTO;
    SoundSettings soundSettings;

    bool operator==(const GuiSettings &other) const
    {
        return other.isLaunchOnStartup == isLaunchOnStartup &&
               other.isAutoConnect == isAutoConnect &&
               other.isHideFromDock == isHideFromDock &&
               other.isShowNotifications == isShowNotifications &&
               other.orderLocation == orderLocation &&
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
               other.trayIconColor == trayIconColor &&
               other.multiDesktopBehavior == multiDesktopBehavior &&
               other.soundSettings == soundSettings;
    }

    bool operator!=(const GuiSettings &other) const
    {
        return !(*this == other);
    }

    void fromIni(QSettings &settings)
    {
        isLaunchOnStartup = settings.value(kIniIsLaunchOnStartupProp, isLaunchOnStartup).toBool();

        settings.beginGroup(QString("Connection"));
        isAutoConnect = settings.value(kIniIsAutoConnectProp, isAutoConnect).toBool();
        isAutoSecureNetworks = settings.value(kIniIsAutoSecureNetworksProp, isAutoSecureNetworks).toBool();

        shareProxyGateway.fromIni(settings);
        splitTunneling.fromIni(settings);
        settings.endGroup();
    }

    void toIni(QSettings &settings) const
    {
        settings.setValue(kIniIsLaunchOnStartupProp, isLaunchOnStartup);

        settings.beginGroup(QString("Connection"));
        settings.setValue(kIniIsAutoConnectProp, isAutoConnect);
        settings.setValue(kIniIsAutoSecureNetworksProp, isAutoSecureNetworks);

        shareProxyGateway.toIni(settings);
        splitTunneling.toIni(settings);
        settings.endGroup();
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonAppSkinProp] = static_cast<int>(appSkin);
        json[kJsonBackgroundSettingsProp] = backgroundSettings.toJson(isForDebugLog);
        json[kJsonIsAutoConnectProp] = isAutoConnect;
        json[kJsonIsAutoSecureNetworksProp] = isAutoSecureNetworks;
        json[kJsonIsDockedToTrayProp] = isDockedToTray;
        json[kJsonIsHideFromDockProp] = isHideFromDock;
        json[kJsonIsLaunchOnStartupProp] = isLaunchOnStartup;
        json[kJsonIsMinimizeAndCloseToTrayProp] = isMinimizeAndCloseToTray;
        json[kJsonIsShowLocationHealthProp] = isShowLocationHealth;
        json[kJsonIsShowNotificationsProp] = isShowNotifications;
        json[kJsonIsStartMinimizedProp] = isStartMinimized;
        json[kJsonOrderLocationProp] = static_cast<int>(orderLocation);
        json[kJsonShareProxyGatewayProp] = shareProxyGateway.toJson(isForDebugLog);
        json[kJsonShareSecureHotspotProp] = shareSecureHotspot.toJson(isForDebugLog);
        json[kJsonSplitTunnelingProp] = splitTunneling.toJson(isForDebugLog);
        json[kJsonTrayIconColourProp] = static_cast<int>(trayIconColor);
        json[kJsonMultiDesktopBehaviourProp] = static_cast<int>(multiDesktopBehavior);
        json[kJsonSoundSettingsProp] = soundSettings.toJson(isForDebugLog);
        json[kJsonVersionProp] = static_cast<int>(versionForSerialization_);

        if (isForDebugLog) {
            json["appSkinDesc"] = APP_SKIN_toString(appSkin);
            json["orderLocationDesc"] = ORDER_LOCATION_TYPE_toString(orderLocation);
            json["trayIconColourDesc"] = TRAY_ICON_COLOR_toString(trayIconColor);
        }

        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const GuiSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.isLaunchOnStartup << o.isAutoConnect << o.isHideFromDock << o.isShowNotifications << o.orderLocation <<
                  o.shareSecureHotspot << o.shareProxyGateway << o.splitTunneling << o.isDockedToTray << o.isMinimizeAndCloseToTray <<
                  o.backgroundSettings << o.isStartMinimized << o.isShowLocationHealth << o.isAutoSecureNetworks << o.appSkin << o.trayIconColor <<
                  o.multiDesktopBehavior << o.soundSettings;

        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, GuiSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isLaunchOnStartup >> o.isAutoConnect >> o.isHideFromDock >> o.isShowNotifications >> o.orderLocation;

        if (version < 6) {
            // latencyDisplay was removed in version 6, ignore it if it exists
            int latencyDisplay;
            stream >> latencyDisplay;
        }

        stream >> o.shareSecureHotspot >> o.shareProxyGateway >> o.splitTunneling >> o.isDockedToTray >> o.isMinimizeAndCloseToTray >>
                  o.backgroundSettings >> o.isStartMinimized >> o.isShowLocationHealth >> o.isAutoSecureNetworks;

        if (version >= 2) {
            stream >> o.appSkin;
        }

        if (version >= 3) {
            stream >> o.trayIconColor;
        }

        if (version >= 4) {
            stream >> o.multiDesktopBehavior;
        }

        if (version >= 5) {
            stream >> o.soundSettings;
        }

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        // Default the tray icon color to OS theme for settings stored prior to us adding the
        // tray icon color preference.
        if (version <= 6) {
            o.trayIconColor = TRAY_ICON_COLOR_OS_THEME;
        }
#endif

        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const GuiSettings &gs)
    {
        QJsonDocument doc(gs.toJson(true));
        QDebugStateSaver saver(dbg);
        dbg.noquote();
        dbg << doc.toJson(QJsonDocument::Compact);
        return dbg;
    }

private:
    static const inline QString kIniIsAutoConnectProp = "Autoconnect";
    static const inline QString kIniIsAutoSecureNetworksProp = "AutosecureNetworks";
    static const inline QString kIniIsLaunchOnStartupProp = "LaunchOnStartup";
    static const inline QString kIniSplitTunnelingProp = "SplitTunneling";

    static const inline QString kJsonAppSkinProp = "appSkin";
    static const inline QString kJsonBackgroundSettingsProp = "backgroundSettings";
    static const inline QString kJsonIsAutoConnectProp = "isAutoConnect";
    static const inline QString kJsonIsAutoSecureNetworksProp = "isAutoSecureNetworks";
    static const inline QString kJsonIsDockedToTrayProp = "isDockedToTray";
    static const inline QString kJsonIsHideFromDockProp = "isHideFromDock";
    static const inline QString kJsonIsLaunchOnStartupProp = "isLaunchOnStartup";
    static const inline QString kJsonIsMinimizeAndCloseToTrayProp = "isMinimizeAndCloseToTray";
    static const inline QString kJsonIsShowLocationHealthProp = "isShowLocationHealth";
    static const inline QString kJsonIsShowNotificationsProp = "isShowNotifications";
    static const inline QString kJsonIsStartMinimizedProp = "isStartMinimized";
    static const inline QString kJsonOrderLocationProp = "orderLocation";
    static const inline QString kJsonShareProxyGatewayProp = "shareProxyGateway";
    static const inline QString kJsonShareSecureHotspotProp = "shareSecureHotspot";
    static const inline QString kJsonSplitTunnelingProp = "splitTunneling";
    static const inline QString kJsonTrayIconColorProp = "trayIconColor";
    static const inline QString kJsonTrayIconColourProp = "trayIconColour";
    static const inline QString kJsonMultiDesktopBehaviorProp = "multiDesktopBehavior";
    static const inline QString kJsonMultiDesktopBehaviourProp = "multiDesktopBehaviour";
    static const inline QString kJsonSoundSettingsProp = "soundSettings";
    static const inline QString kJsonVersionProp = "version";

    static constexpr quint32 versionForSerialization_ = 7;  // should increment the version if the data format is changed
};

inline GuiSettings::GuiSettings(const QJsonObject &json)
{
    if (json.contains(kJsonAppSkinProp) && json[kJsonAppSkinProp].isDouble()) {
        appSkin = APP_SKIN_fromInt(json[kJsonAppSkinProp].toInt());
    }

    if (json.contains(kJsonBackgroundSettingsProp) && json[kJsonBackgroundSettingsProp].isObject()) {
        backgroundSettings = types::BackgroundSettings(json[kJsonBackgroundSettingsProp].toObject());
    }

    if (json.contains(kJsonIsAutoConnectProp) && json[kJsonIsAutoConnectProp].isBool()) {
        isAutoConnect = json[kJsonIsAutoConnectProp].toBool();
    }

    if (json.contains(kJsonIsAutoSecureNetworksProp) && json[kJsonIsAutoSecureNetworksProp].isBool()) {
        isAutoSecureNetworks = json[kJsonIsAutoSecureNetworksProp].toBool();
    }

#if !defined(Q_OS_LINUX)
    if (json.contains(kJsonIsDockedToTrayProp) && json[kJsonIsDockedToTrayProp].isBool()) {
        isDockedToTray = json[kJsonIsDockedToTrayProp].toBool();
    }
#endif

#if defined(Q_OS_MACOS)
    if (json.contains(kJsonIsHideFromDockProp) && json[kJsonIsHideFromDockProp].isBool()) {
        isHideFromDock = json[kJsonIsHideFromDockProp].toBool();
    }
#endif

    if (json.contains(kJsonIsLaunchOnStartupProp) && json[kJsonIsLaunchOnStartupProp].isBool()) {
        isLaunchOnStartup = json[kJsonIsLaunchOnStartupProp].toBool();
    }

    if (json.contains(kJsonIsMinimizeAndCloseToTrayProp) && json[kJsonIsMinimizeAndCloseToTrayProp].isBool()) {
        isMinimizeAndCloseToTray = json[kJsonIsMinimizeAndCloseToTrayProp].toBool();
    }

    if (json.contains(kJsonIsShowLocationHealthProp) && json[kJsonIsShowLocationHealthProp].isBool()) {
        isShowLocationHealth = json[kJsonIsShowLocationHealthProp].toBool();
    }

    if (json.contains(kJsonIsShowNotificationsProp) && json[kJsonIsShowNotificationsProp].isBool()) {
        isShowNotifications = json[kJsonIsShowNotificationsProp].toBool();
    }

    if (json.contains(kJsonIsStartMinimizedProp) && json[kJsonIsStartMinimizedProp].isBool()) {
        isStartMinimized = json[kJsonIsStartMinimizedProp].toBool();
    }

    if (json.contains(kJsonOrderLocationProp) && json[kJsonOrderLocationProp].isDouble()) {
        orderLocation = ORDER_LOCATION_TYPE_fromInt(json[kJsonOrderLocationProp].toInt());
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    if (json.contains(kJsonTrayIconColourProp) && json[kJsonTrayIconColourProp].isDouble()) {
        trayIconColor = TRAY_ICON_COLOR_fromInt(json[kJsonTrayIconColourProp].toInt());
    } else if (json.contains(kJsonTrayIconColorProp) && json[kJsonTrayIconColorProp].isDouble()) {
        trayIconColor = TRAY_ICON_COLOR_fromInt(json[kJsonTrayIconColorProp].toInt());
    }
#endif

#if defined(Q_OS_WIN)
    if (json.contains(kJsonShareSecureHotspotProp) && json[kJsonShareSecureHotspotProp].isObject()) {
        shareSecureHotspot = types::ShareSecureHotspot(json[kJsonShareSecureHotspotProp].toObject());
    }
#endif

    if (json.contains(kJsonShareProxyGatewayProp) && json[kJsonShareProxyGatewayProp].isObject()) {
        shareProxyGateway = types::ShareProxyGateway(json[kJsonShareProxyGatewayProp].toObject());
    }

    if (json.contains(kJsonSplitTunnelingProp) && json[kJsonSplitTunnelingProp].isObject()) {
        splitTunneling = types::SplitTunneling(json[kJsonSplitTunnelingProp].toObject());
    }

#if defined(Q_OS_MACOS)
    if (json.contains(kJsonMultiDesktopBehaviourProp) && json[kJsonMultiDesktopBehaviourProp].isDouble()) {
        multiDesktopBehavior = MULTI_DESKTOP_BEHAVIOR_fromInt(json[kJsonMultiDesktopBehaviourProp].toInt());
    } else if (json.contains(kJsonMultiDesktopBehaviorProp) && json[kJsonMultiDesktopBehaviorProp].isDouble()) {
        multiDesktopBehavior = MULTI_DESKTOP_BEHAVIOR_fromInt(json[kJsonMultiDesktopBehaviorProp].toInt());
    }
#endif

    if (json.contains(kJsonSoundSettingsProp) && json[kJsonSoundSettingsProp].isObject()) {
        soundSettings = types::SoundSettings(json[kJsonSoundSettingsProp].toObject());
    }
}

} // types namespace
