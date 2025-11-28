#include "enums.h"
#include <QList>
#include <QObject>
#include <QMetaType>
#include "utils/ws_assert.h"

const int typeIdOpenVpnError = qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
const int typeIdProxyOption = qRegisterMetaType<PROXY_OPTION>("PROXY_OPTION");
const int typeIdLoginMessage = qRegisterMetaType<LOGIN_MESSAGE>("LOGIN_MESSAGE");
const int typeIdApiRetCode = qRegisterMetaType<API_RET_CODE>("API_RET_CODE");
const int typeIdEngineInitRetCode = qRegisterMetaType<ENGINE_INIT_RET_CODE>("ENGINE_INIT_RET_CODE");
const int typeIdConnectState = qRegisterMetaType<CONNECT_STATE>("CONNECT_STATE");
const int typeIdDisconnectReason = qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");
const int typeIdProxySharingType = qRegisterMetaType<PROXY_SHARING_TYPE>("PROXY_SHARING_TYPE");
const int typeIdWebSessionPurpose = qRegisterMetaType<WEB_SESSION_PURPOSE>("WEB_SESSION_PURPOSE");


DNS_POLICY_TYPE DNS_POLICY_TYPE_fromInt(int t)
{
    if (t == 0) return DNS_TYPE_OS_DEFAULT;
    else if (t == 1) return DNS_TYPE_OPEN_DNS;
    else if (t == 2) return DNS_TYPE_CLOUDFLARE;
    else if (t == 3) return DNS_TYPE_GOOGLE;
    else if (t == 4) return DNS_TYPE_CONTROLD;
    else {
        WS_ASSERT(false);
        return DNS_TYPE_OS_DEFAULT;
    }
}

DNS_POLICY_TYPE DNS_POLICY_TYPE_fromString(const QString &s)
{
    if (s == "OS Default") {
        return DNS_TYPE_OS_DEFAULT;
    } else if (s == "OpenDNS") {
        return DNS_TYPE_OPEN_DNS;
    } else if (s == "Cloudflare") {
        return DNS_TYPE_CLOUDFLARE;
    } else if (s == "Google") {
        return DNS_TYPE_GOOGLE;
    } else if (s == "Control D") {
        return DNS_TYPE_CONTROLD;
    } else {
        WS_ASSERT(false);
        return DNS_TYPE_OS_DEFAULT;
    }
}

QString DNS_POLICY_TYPE_toString(DNS_POLICY_TYPE d)
{
    if (d == DNS_TYPE_OS_DEFAULT) {
        return QObject::tr("OS Default");
    } else if (d == DNS_TYPE_OPEN_DNS) {
        return "OpenDNS";
    } else if (d == DNS_TYPE_CLOUDFLARE) {
        return "Cloudflare";
    } else if (d == DNS_TYPE_GOOGLE) {
        return "Google";
    } else if (d == DNS_TYPE_CONTROLD) {
        return "Control D";
    } else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

PROXY_SHARING_TYPE PROXY_SHARING_TYPE_fromInt(int t)
{
    if (t == 0) return PROXY_SHARING_HTTP;
    else if (t == 1) return PROXY_SHARING_SOCKS;
    else {
        WS_ASSERT(false);
        return PROXY_SHARING_HTTP;
    }
}

PROXY_SHARING_TYPE PROXY_SHARING_TYPE_fromString(const QString &s)
{
    if (s == "HTTP") return PROXY_SHARING_HTTP;
    else if (s == "SOCKS") return PROXY_SHARING_SOCKS;
    else {
        WS_ASSERT(false);
        return PROXY_SHARING_HTTP;
    }
}

QString PROXY_SHARING_TYPE_toString(PROXY_SHARING_TYPE t)
{
    if (t == PROXY_SHARING_HTTP) return "HTTP";
    else if (t == PROXY_SHARING_SOCKS) return "SOCKS";
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

ORDER_LOCATION_TYPE ORDER_LOCATION_TYPE_fromInt(int t)
{
    if (t == 0) return ORDER_LOCATION_BY_GEOGRAPHY;
    else if (t == 1) return ORDER_LOCATION_BY_ALPHABETICALLY;
    else if (t == 2) return ORDER_LOCATION_BY_LATENCY;
    else {
        WS_ASSERT(false);
        return ORDER_LOCATION_BY_GEOGRAPHY;
    }
}

QString ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_TYPE p)
{
    if (p == ORDER_LOCATION_BY_GEOGRAPHY) return QObject::tr("Geography");
    else if (p == ORDER_LOCATION_BY_ALPHABETICALLY) return QObject::tr("Alphabet");
    else if (p == ORDER_LOCATION_BY_LATENCY) return QObject::tr("Latency");
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

BACKGROUND_TYPE BACKGROUND_TYPE_fromInt(int t)
{
    if (t == 0) return BACKGROUND_TYPE_NONE;
    else if (t == 1) return BACKGROUND_TYPE_COUNTRY_FLAGS;
    else if (t == 2) return BACKGROUND_TYPE_CUSTOM;
    else if (t == 3) return BACKGROUND_TYPE_BUNDLED;
    else {
        WS_ASSERT(false);
        return BACKGROUND_TYPE_NONE;
    }
}

SOUND_NOTIFICATION_TYPE SOUND_NOTIFICATION_TYPE_fromInt(int t)
{
    if (t == 0) return SOUND_NOTIFICATION_TYPE_NONE;
    else if (t == 1) return SOUND_NOTIFICATION_TYPE_BUNDLED;
    else if (t == 2) return SOUND_NOTIFICATION_TYPE_CUSTOM;
    else {
        WS_ASSERT(false);
        return SOUND_NOTIFICATION_TYPE_NONE;
    }
}

QString TAP_ADAPTER_TYPE_toString(TAP_ADAPTER_TYPE t)
{
    if (t == DCO_ADAPTER) return "DCO";
    else if (t == WINTUN_ADAPTER) return "Wintun";
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

FIREWALL_MODE FIREWALL_MODE_fromInt(int t)
{
    if (t == 0) return FIREWALL_MODE_MANUAL;
    else if (t == 1) return FIREWALL_MODE_AUTOMATIC;
    else if (t == 2) return FIREWALL_MODE_ALWAYS_ON;
    else if (t == 3) return FIREWALL_MODE_ALWAYS_ON_PLUS;
    else {
        WS_ASSERT(false);
        return FIREWALL_MODE_AUTOMATIC;
    }

}

FIREWALL_MODE FIREWALL_MODE_fromString(const QString &s)
{
    if (s == "Manual") return FIREWALL_MODE_MANUAL;
    else if (s == "Auto") return FIREWALL_MODE_AUTOMATIC;
    else if (s == "Always On") return FIREWALL_MODE_ALWAYS_ON;
    else if (s == "Always On+") return FIREWALL_MODE_ALWAYS_ON_PLUS;
    else {
        WS_ASSERT(false);
        return FIREWALL_MODE_AUTOMATIC;
    }
}

QString FIREWALL_MODE_toString(FIREWALL_MODE t)
{
    if (t == FIREWALL_MODE_MANUAL) return QObject::tr("Manual");
    else if (t == FIREWALL_MODE_AUTOMATIC) return QObject::tr("Auto");
    else if (t == FIREWALL_MODE_ALWAYS_ON) return QObject::tr("Always On");
    else if (t == FIREWALL_MODE_ALWAYS_ON_PLUS) return QObject::tr("Always On+");
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

QList<QPair<QString, QVariant>> FIREWALL_MODE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_MANUAL), FIREWALL_MODE_MANUAL);
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_AUTOMATIC), FIREWALL_MODE_AUTOMATIC);
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_ALWAYS_ON), FIREWALL_MODE_ALWAYS_ON);
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_ALWAYS_ON_PLUS), FIREWALL_MODE_ALWAYS_ON_PLUS);
    return l;
}

FIREWALL_WHEN FIREWALL_WHEN_fromInt(int t)
{
    if (t == 0) return FIREWALL_WHEN_BEFORE_CONNECTION;
    else if (t == 1) return FIREWALL_WHEN_AFTER_CONNECTION;
    else {
        WS_ASSERT(false);
        return FIREWALL_WHEN_BEFORE_CONNECTION;
    }
}

FIREWALL_WHEN FIREWALL_WHEN_fromString(const QString &s)
{
    if (s == "Before Connection") return FIREWALL_WHEN_BEFORE_CONNECTION;
    else if (s == "After Connection") return FIREWALL_WHEN_AFTER_CONNECTION;
    else {
        WS_ASSERT(false);
        return FIREWALL_WHEN_BEFORE_CONNECTION;
    }
}

QString FIREWALL_WHEN_toString(FIREWALL_WHEN t)
{
    if (t == FIREWALL_WHEN_BEFORE_CONNECTION) return QObject::tr("Before Connection");
    else if (t == FIREWALL_WHEN_AFTER_CONNECTION) return QObject::tr("After Connection");
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

NETWORK_INTERFACE_TYPE NETWORK_INTERFACE_TYPE_fromInt(int t)
{
    if (t == 0) return NETWORK_INTERFACE_NONE;
    else if (t == 1) return NETWORK_INTERFACE_ETH;
    else if (t == 2) return NETWORK_INTERFACE_WIFI;
    else if (t == 3) return NETWORK_INTERFACE_PPP;
    else {
        WS_ASSERT(false);
        return NETWORK_INTERFACE_NONE;
    }
}

NETWORK_TRUST_TYPE NETWORK_TRUST_TYPE_fromInt(int t)
{
    if (t == 0) return NETWORK_TRUST_SECURED;
    else if (t == 1) return NETWORK_TRUST_UNSECURED;
    else {
        // NETWORK_TRUST_FORGET is a transitory state and should never be here
        WS_ASSERT(false);
        return NETWORK_TRUST_SECURED;
    }
}

NETWORK_TRUST_TYPE NETWORK_TRUST_TYPE_fromString(const QString &s)
{
    if (s == "Secured") return NETWORK_TRUST_SECURED;
    else if (s == "Unsecured") return NETWORK_TRUST_UNSECURED;
    else {
        // NETWORK_TRUST_FORGET is a transitory state and should never be here
        WS_ASSERT(false);
        return NETWORK_TRUST_SECURED;
    }
}

QString NETWORK_TRUST_TYPE_toString(NETWORK_TRUST_TYPE t)
{
    if (t == NETWORK_TRUST_SECURED) return "Secured";
    else if (t == NETWORK_TRUST_UNSECURED) return "Unsecured";
    else {
        // NETWORK_TRUST_FORGET is a transitory state and should never be here
        WS_ASSERT(false);
        return "Secured";
    }
}

QList<QPair<QString, QVariant>> FIREWALL_WHEN_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(FIREWALL_WHEN_toString(FIREWALL_WHEN_BEFORE_CONNECTION), FIREWALL_WHEN_BEFORE_CONNECTION);
    l << qMakePair(FIREWALL_WHEN_toString(FIREWALL_WHEN_AFTER_CONNECTION), FIREWALL_WHEN_AFTER_CONNECTION);
    return l;
}

QList<QPair<QString, QVariant>> PROXY_SHARING_TYPE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(PROXY_SHARING_TYPE_toString(PROXY_SHARING_HTTP), PROXY_SHARING_HTTP);
    l << qMakePair(PROXY_SHARING_TYPE_toString(PROXY_SHARING_SOCKS), PROXY_SHARING_SOCKS);
    return l;
}

PROXY_OPTION PROXY_OPTION_fromInt(int t)
{
    if (t == 0) return PROXY_OPTION_NONE;
    else if (t == 1) return PROXY_OPTION_AUTODETECT;
    else if (t == 2) return PROXY_OPTION_HTTP;
    else if (t == 3) return PROXY_OPTION_SOCKS;
    else {
        WS_ASSERT(false);
        return PROXY_OPTION_NONE;
    }
}

PROXY_OPTION PROXY_OPTION_fromString(const QString &s)
{
    if (s == "None") return PROXY_OPTION_NONE;
    else if (s == "Auto-detect") return PROXY_OPTION_AUTODETECT;
    else if (s == "HTTP") return PROXY_OPTION_HTTP;
    else if (s == "SOCKS") return PROXY_OPTION_SOCKS;
    else {
        WS_ASSERT(false);
        return PROXY_OPTION_NONE;
    }

}

QString PROXY_OPTION_toString(PROXY_OPTION t)
{
    if (t == PROXY_OPTION_NONE) return QObject::tr("None");
    else if (t == PROXY_OPTION_AUTODETECT) return QObject::tr("Auto-detect");
    else if (t == PROXY_OPTION_HTTP) return "HTTP";
    else if (t == PROXY_OPTION_SOCKS) return "SOCKS";
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

QList<QPair<QString, QVariant>> PROXY_OPTION_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_NONE), PROXY_OPTION_NONE);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_AUTODETECT), PROXY_OPTION_AUTODETECT);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_HTTP), PROXY_OPTION_HTTP);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_SOCKS), PROXY_OPTION_SOCKS);
    return l;
}

QList<QPair<QString, QVariant>> DNS_POLICY_TYPE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(DNS_POLICY_TYPE_toString(DNS_TYPE_OS_DEFAULT), DNS_TYPE_OS_DEFAULT);
    l << qMakePair(DNS_POLICY_TYPE_toString(DNS_TYPE_OPEN_DNS), DNS_TYPE_OPEN_DNS);
    l << qMakePair(DNS_POLICY_TYPE_toString(DNS_TYPE_CLOUDFLARE), DNS_TYPE_CLOUDFLARE);
    l << qMakePair(DNS_POLICY_TYPE_toString(DNS_TYPE_GOOGLE), DNS_TYPE_GOOGLE);
    l << qMakePair(DNS_POLICY_TYPE_toString(DNS_TYPE_CONTROLD), DNS_TYPE_CONTROLD);
    return l;
}

QList<QPair<QString, QVariant>> ORDER_LOCATION_TYPE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_GEOGRAPHY), ORDER_LOCATION_BY_GEOGRAPHY);
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_ALPHABETICALLY), ORDER_LOCATION_BY_ALPHABETICALLY);
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_LATENCY), ORDER_LOCATION_BY_LATENCY);
    return l;

}

UPDATE_CHANNEL UPDATE_CHANNEL_fromInt(int t)
{
    if (t == 0) return UPDATE_CHANNEL_RELEASE;
    else if (t == 1) return UPDATE_CHANNEL_BETA;
    else if (t == 2) return UPDATE_CHANNEL_GUINEA_PIG;
    else if (t == 3) return UPDATE_CHANNEL_INTERNAL;
    else {
        WS_ASSERT(false);
        return UPDATE_CHANNEL_RELEASE;
    }
}

QString UPDATE_CHANNEL_toString(UPDATE_CHANNEL t)
{
    if (t == UPDATE_CHANNEL_RELEASE) return QObject::tr("Release");
    else if (t == UPDATE_CHANNEL_BETA) return QObject::tr("Beta");
    else if (t == UPDATE_CHANNEL_GUINEA_PIG) return QObject::tr("Guinea Pig");
    else if (t == UPDATE_CHANNEL_INTERNAL) return QObject::tr("Internal");
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

UPDATE_CHANNEL UPDATE_CHANNEL_fromString(const QString &s)
{
    if (s == "Release") return UPDATE_CHANNEL_RELEASE;
    else if (s == "Beta") return UPDATE_CHANNEL_BETA;
    else if (s == "Guinea Pig") return UPDATE_CHANNEL_GUINEA_PIG;
    else if (s == "Internal") return UPDATE_CHANNEL_INTERNAL;
    else {
        WS_ASSERT(false);
        return UPDATE_CHANNEL_RELEASE;
    }
}

QList<QPair<QString, QVariant>> UPDATE_CHANNEL_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_RELEASE), UPDATE_CHANNEL_RELEASE);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_BETA), UPDATE_CHANNEL_BETA);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_GUINEA_PIG), UPDATE_CHANNEL_GUINEA_PIG);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_INTERNAL), UPDATE_CHANNEL_INTERNAL);
    return l;
}

DNS_MANAGER_TYPE DNS_MANAGER_TYPE_fromInt(int t)
{
    if (t == 0) return DNS_MANAGER_AUTOMATIC;
    else if (t == 1) return DNS_MANAGER_RESOLV_CONF;
    else if (t == 2) return DNS_MANAGER_SYSTEMD_RESOLVED;
    else if (t == 3) return DNS_MANAGER_NETWORK_MANAGER;
    else {
        WS_ASSERT(false);
        return DNS_MANAGER_AUTOMATIC;
    }
}

QString DNS_MANAGER_TYPE_toString(DNS_MANAGER_TYPE t)
{
    if (t == DNS_MANAGER_AUTOMATIC) return QObject::tr("Auto");
    else if (t == DNS_MANAGER_RESOLV_CONF) return "resolvconf";
    else if (t == DNS_MANAGER_SYSTEMD_RESOLVED) return "systemd-resolved";
    else if (t == DNS_MANAGER_NETWORK_MANAGER) return "NetworkManager";
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

DNS_MANAGER_TYPE DNS_MANAGER_TYPE_fromString(const QString &s)
{
    if (s == "Auto") return DNS_MANAGER_AUTOMATIC;
    else if (s == "resolvconf") return DNS_MANAGER_RESOLV_CONF;
    else if (s == "systemd-resolved") return DNS_MANAGER_SYSTEMD_RESOLVED;
    else if (s == "NetworkManager") return DNS_MANAGER_NETWORK_MANAGER;
    else {
        WS_ASSERT(false);
        return DNS_MANAGER_AUTOMATIC;
    }
}

QList<QPair<QString, QVariant>> DNS_MANAGER_TYPE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_AUTOMATIC), DNS_MANAGER_AUTOMATIC);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_RESOLV_CONF), DNS_MANAGER_RESOLV_CONF);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_SYSTEMD_RESOLVED), DNS_MANAGER_SYSTEMD_RESOLVED);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_NETWORK_MANAGER), DNS_MANAGER_NETWORK_MANAGER);
    return l;

}

CONNECTED_DNS_TYPE CONNECTED_DNS_TYPE_fromInt(int t)
{
    if (t == 0) return CONNECTED_DNS_TYPE_AUTO;
    else if (t == 1) return CONNECTED_DNS_TYPE_CUSTOM;
    else if (t == 2) return CONNECTED_DNS_TYPE_FORCED;
    else if (t == 3) return CONNECTED_DNS_TYPE_LOCAL;
    else if (t == 4) return CONNECTED_DNS_TYPE_CONTROLD;
    else {
        WS_ASSERT(false);
        return CONNECTED_DNS_TYPE_AUTO;
    }
}

QString CONNECTED_DNS_TYPE_toString(CONNECTED_DNS_TYPE t)
{
    if (t == CONNECTED_DNS_TYPE_AUTO) {
        return QObject::tr("Auto");
    }
    else if (t == CONNECTED_DNS_TYPE_CUSTOM) {
        return QObject::tr("Custom");
    }
    else if (t == CONNECTED_DNS_TYPE_FORCED) {
        return QObject::tr("Forced");
    }
    else if (t == CONNECTED_DNS_TYPE_LOCAL) {
        return QObject::tr("Local DNS");
    }
    else if (t == CONNECTED_DNS_TYPE_CONTROLD) {
        return "Control D";
    }
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

CONNECTED_DNS_TYPE CONNECTED_DNS_TYPE_fromString(const QString &s)
{
    if (s == "Auto") {
        return CONNECTED_DNS_TYPE_AUTO;
    }
    else if (s == "Custom") {
        return CONNECTED_DNS_TYPE_CUSTOM;
    }
    else if (s == "Forced") {
        return CONNECTED_DNS_TYPE_FORCED;
    }
    else if (s == "Local DNS") {
        return CONNECTED_DNS_TYPE_LOCAL;
    }
    else if (s == "Control D") {
        return CONNECTED_DNS_TYPE_CONTROLD;
    }
    else {
        WS_ASSERT(false);
        return CONNECTED_DNS_TYPE_AUTO;
    }
}

SPLIT_TUNNELING_MODE SPLIT_TUNNELING_MODE_fromInt(int t)
{
    if (t == 0) return SPLIT_TUNNELING_MODE_EXCLUDE;
    else if (t == 1) return SPLIT_TUNNELING_MODE_INCLUDE;
    else {
        WS_ASSERT(false);
        return SPLIT_TUNNELING_MODE_EXCLUDE;
    }
}

SPLIT_TUNNELING_MODE SPLIT_TUNNELING_MODE_fromString(const QString &s)
{
    if (s == "Exclude") {
        return SPLIT_TUNNELING_MODE_EXCLUDE;
    }
    else if (s == "Include") {
        return SPLIT_TUNNELING_MODE_INCLUDE;
    }
    else {
        WS_ASSERT(false);
        return SPLIT_TUNNELING_MODE_EXCLUDE;
    }
}

QString SPLIT_TUNNELING_MODE_toString(SPLIT_TUNNELING_MODE t)
{
    if (t == SPLIT_TUNNELING_MODE_EXCLUDE) {
        return QObject::tr("Exclude");
    }
    else if (t == SPLIT_TUNNELING_MODE_INCLUDE) {
        return QObject::tr("Include");
    }
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

SPLIT_TUNNELING_NETWORK_ROUTE_TYPE SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_fromInt(int t)
{
    if (t == 0) return SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    else if (t == 1) return SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME;
    else {
        WS_ASSERT(false);
        return SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    }
}

SPLIT_TUNNELING_APP_TYPE SPLIT_TUNNELING_APP_TYPE_fromInt(int t)
{
    if (t == 0) return SPLIT_TUNNELING_APP_TYPE_USER;
    else if (t == 1) return SPLIT_TUNNELING_APP_TYPE_SYSTEM;
    else {
        WS_ASSERT(false);
        return SPLIT_TUNNELING_APP_TYPE_USER;
    }
}

APP_SKIN APP_SKIN_fromInt(int t)
{
    if (t == 0) return APP_SKIN_ALPHA;
    else if (t == 1) return APP_SKIN_VAN_GOGH;
    else {
        WS_ASSERT(false);
        return APP_SKIN_ALPHA;
    }
}

QList<QPair<QString, QVariant>> APP_SKIN_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(APP_SKIN_toString(APP_SKIN_ALPHA), APP_SKIN_ALPHA);
    l << qMakePair(APP_SKIN_toString(APP_SKIN_VAN_GOGH), APP_SKIN_VAN_GOGH);
    return l;
}

QString APP_SKIN_toString(APP_SKIN s)
{
    if (s == APP_SKIN_ALPHA) {
        return QObject::tr("Alpha");
    }
    else if (s == APP_SKIN_VAN_GOGH) {
        return QObject::tr("Van Gogh");
    }
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

TRAY_ICON_COLOR TRAY_ICON_COLOR_default()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    return TRAY_ICON_COLOR_OS_THEME;
#else
    return TRAY_ICON_COLOR_WHITE;
#endif
}

TRAY_ICON_COLOR TRAY_ICON_COLOR_fromInt(int t)
{
    if (t == 0) return TRAY_ICON_COLOR_WHITE;
    else if (t == 1) return TRAY_ICON_COLOR_BLACK;
    else if (t == 2) {
#if defined(Q_OS_LINUX)
        // Revert to the default on Linux, without asserting, if user is importing this setting from another OS.
        return TRAY_ICON_COLOR_default();
#else
        return TRAY_ICON_COLOR_OS_THEME;
#endif
    } else {
        WS_ASSERT(false);
        return TRAY_ICON_COLOR_default();
    }
}

QString TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR c)
{
    if (c == TRAY_ICON_COLOR_WHITE) {
        return QObject::tr("White");
    }
    else if (c == TRAY_ICON_COLOR_BLACK) {
        return QObject::tr("Black");
    }
#if !defined(Q_OS_LINUX)
    else if (c == TRAY_ICON_COLOR_OS_THEME) {
        return QObject::tr("OS Default");
    }
#endif
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

QList<QPair<QString, QVariant>> TRAY_ICON_COLOR_toList()
{
    QList<QPair<QString, QVariant>> l;
#if defined(Q_OS_WIN)
    l << qMakePair(TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR_OS_THEME), TRAY_ICON_COLOR_OS_THEME);
#endif
    l << qMakePair(TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR_WHITE), TRAY_ICON_COLOR_WHITE);
    l << qMakePair(TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR_BLACK), TRAY_ICON_COLOR_BLACK);
    return l;
}

TOGGLE_MODE TOGGLE_MODE_fromString(const QString &s)
{
    if (s == "Auto") {
        return TOGGLE_MODE_AUTO;
    } else if (s == "Manual") {
       return TOGGLE_MODE_MANUAL;
    } else {
        WS_ASSERT(false);
        return TOGGLE_MODE_AUTO;
    }
}

QString TOGGLE_MODE_toString(TOGGLE_MODE t)
{
    if (t == TOGGLE_MODE_AUTO) {
        return "Auto";
    } else if (t == TOGGLE_MODE_MANUAL) {
        return "Manual";
    } else {
         WS_ASSERT(false);
        return "Auto";
    }
}

MULTI_DESKTOP_BEHAVIOR MULTI_DESKTOP_BEHAVIOR_fromInt(int t)
{
    if (t == 0) return MULTI_DESKTOP_AUTO;
    else if (t == 1) return MULTI_DESKTOP_DUPLICATE;
    else if (t == 2) return MULTI_DESKTOP_MOVE_SPACES;
    else if (t == 3) return MULTI_DESKTOP_MOVE_WINDOW;
    else {
        WS_ASSERT(false);
        return MULTI_DESKTOP_AUTO;
    }
}

QString MULTI_DESKTOP_BEHAVIOR_toString(MULTI_DESKTOP_BEHAVIOR m)
{
    if (m == MULTI_DESKTOP_AUTO) {
        return QObject::tr("Auto");
    } else if (m == MULTI_DESKTOP_DUPLICATE) {
        return QObject::tr("Duplicate");
    } else if (m == MULTI_DESKTOP_MOVE_SPACES) {
        return QObject::tr("Move spaces");
    } else if (m == MULTI_DESKTOP_MOVE_WINDOW) {
        return QObject::tr("Move window");
    } else {
        WS_ASSERT(false);
        return QObject::tr("Auto");
    }
}

QList<QPair<QString, QVariant>> MULTI_DESKTOP_BEHAVIOR_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(MULTI_DESKTOP_BEHAVIOR_toString(MULTI_DESKTOP_AUTO), MULTI_DESKTOP_AUTO);
    l << qMakePair(MULTI_DESKTOP_BEHAVIOR_toString(MULTI_DESKTOP_DUPLICATE), MULTI_DESKTOP_DUPLICATE);
    l << qMakePair(MULTI_DESKTOP_BEHAVIOR_toString(MULTI_DESKTOP_MOVE_SPACES), MULTI_DESKTOP_MOVE_SPACES);
    l << qMakePair(MULTI_DESKTOP_BEHAVIOR_toString(MULTI_DESKTOP_MOVE_WINDOW), MULTI_DESKTOP_MOVE_WINDOW);
    return l;
}

ASPECT_RATIO_MODE ASPECT_RATIO_MODE_fromInt(int t)
{
    if (t == 0) return ASPECT_RATIO_MODE_STRETCH;
    else if (t == 1) return ASPECT_RATIO_MODE_FILL;
    else if (t == 2) return ASPECT_RATIO_MODE_TILE;
    else {
        WS_ASSERT(false);
        return ASPECT_RATIO_MODE_STRETCH;
    }
}

QString ASPECT_RATIO_MODE_toString(ASPECT_RATIO_MODE t)
{
    if (t == ASPECT_RATIO_MODE_STRETCH) {
        return QObject::tr("Stretch");
    } else if (t == ASPECT_RATIO_MODE_FILL) {
        return QObject::tr("Fill");
    } else if (t == ASPECT_RATIO_MODE_TILE) {
        return QObject::tr("Tile");
    } else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

QList<QPair<QString, QVariant>> ASPECT_RATIO_MODE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(ASPECT_RATIO_MODE_toString(ASPECT_RATIO_MODE_STRETCH), ASPECT_RATIO_MODE_STRETCH);
    l << qMakePair(ASPECT_RATIO_MODE_toString(ASPECT_RATIO_MODE_FILL), ASPECT_RATIO_MODE_FILL);
    l << qMakePair(ASPECT_RATIO_MODE_toString(ASPECT_RATIO_MODE_TILE), ASPECT_RATIO_MODE_TILE);
    return l;
}
