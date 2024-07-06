#include "enums.h"
#include <QList>
#include <QObject>
#include <QMetaType>
#include "utils/ws_assert.h"

const int typeIdOpenVpnError = qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
const int typeIdProxyOption = qRegisterMetaType<PROXY_OPTION>("PROXY_OPTION");
const int typeIdLoginRet = qRegisterMetaType<LOGIN_RET>("LOGIN_RET");
const int typeIdLoginMessage = qRegisterMetaType<LOGIN_MESSAGE>("LOGIN_MESSAGE");
const int typeIdServerApiRetCode = qRegisterMetaType<SERVER_API_RET_CODE>("SERVER_API_RET_CODE");
const int typeIdEngineInitRetCode = qRegisterMetaType<ENGINE_INIT_RET_CODE>("ENGINE_INIT_RET_CODE");
const int typeIdConnectState = qRegisterMetaType<CONNECT_STATE>("CONNECT_STATE");
const int typeIdDisconnectReason = qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");
const int typeIdProxySharingType = qRegisterMetaType<PROXY_SHARING_TYPE>("PROXY_SHARING_TYPE");
const int typeIdWebSessionPurpose = qRegisterMetaType<WEB_SESSION_PURPOSE>("WEB_SESSION_PURPOSE");

QString LOGIN_RET_toString(LOGIN_RET ret)
{
    if (ret == LOGIN_RET_SUCCESS) {
        return "SUCCESS";
    } else if (ret == LOGIN_RET_NO_API_CONNECTIVITY) {
        return "NO_API_CONNECTIVITY";
    } else if (ret == LOGIN_RET_NO_CONNECTIVITY) {
        return "NO_CONNECTIVITY";
    } else if (ret == LOGIN_RET_INCORRECT_JSON) {
        return "INCORRECT_JSON";
    } else if (ret == LOGIN_RET_BAD_USERNAME) {
        return "BAD_USERNAME";
    } else if (ret == LOGIN_RET_SSL_ERROR) {
        return "SSL_ERROR";
    } else if (ret == LOGIN_RET_BAD_CODE2FA) {
        return "BAD_CODE2FA";
    } else if (ret == LOGIN_RET_MISSING_CODE2FA) {
        return "MISSING_CODE2FA";
    } else if (ret == LOGIN_RET_ACCOUNT_DISABLED) {
        return "ACCOUNT_DISABLED";
    } else if (ret == LOGIN_RET_SESSION_INVALID) {
        return "SESSION_INVALID";
    } else if (ret == LOGIN_RET_RATE_LIMITED) {
        return "RATE_LIMITED";
    } else {
        WS_ASSERT(false);
        return "UNKNOWN";
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

QString LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_TYPE t)
{
    if (t == LATENCY_DISPLAY_BARS) return QObject::tr("Bars");
    else if (t == LATENCY_DISPLAY_MS) return QObject::tr("Ms");
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
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

FIREWALL_MODE FIREWALL_MODE_fromString(const QString &s)
{
    if (s == "Manual") return FIREWALL_MODE_MANUAL;
    else if (s == "Auto") return FIREWALL_MODE_AUTOMATIC;
    else if (s == "Always On") return FIREWALL_MODE_ALWAYS_ON;
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
    return l;
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

QList<QPair<QString, QVariant>> LATENCY_DISPLAY_TYPE_toList()
{
    QList<QPair<QString, QVariant>> l;
    l << qMakePair(LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_BARS), LATENCY_DISPLAY_BARS);
    l << qMakePair(LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_MS), LATENCY_DISPLAY_MS);
    return l;
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
    else {
        WS_ASSERT(false);
        return CONNECTED_DNS_TYPE_AUTO;
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

QString TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR c)
{
    if (c == TRAY_ICON_COLOR_WHITE) {
        return QObject::tr("White");
    }
    else if (c == TRAY_ICON_COLOR_BLACK) {
        return QObject::tr("Black");
    }
    else {
        WS_ASSERT(false);
        return QObject::tr("UNKNOWN");
    }
}

QList<QPair<QString, QVariant>> TRAY_ICON_COLOR_toList()
{
    QList<QPair<QString, QVariant>> l;
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

