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
const int typeIdSplitTunnelStartFailReason = qRegisterMetaType<SPLIT_TUNNEL_START_FAIL_REASON>("SPLIT_TUNNEL_START_FAIL_REASON");


// Table-driven implementation of the generic enum <-> int/string helpers declared in enums.h.
//
// Every enum is described once by an EnumInfo specialization: a {value, string, translate} table
// plus its defaults/fallback.  This table is the single source of truth for all conversions.  The
// translatable strings are marked with QT_TRANSLATE_NOOP("QObject", ...) so that lupdate keeps
// extracting them under the same "QObject" context the original QObject::tr(...) calls used --
// existing translations stay intact.  Numeric values are never altered, so serialization of these
// enums is unaffected.
//
// At the bottom of this file the generic templates are explicitly instantiated for exactly the set
// of enum/operation combinations that are used elsewhere; that is what callers link against.
namespace {

template <typename E>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct EnumDesc
{
    E value;
    const char *str;    // wrapped in QT_TRANSLATE_NOOP when 'translate' is true; "" when unused
    bool translate;
};

// Specialized below for every enum that supports conversions.  Each specialization provides the
// members required by the operations that enum needs:
//   table              - always
//   kDefaultInt        - if enumFromInt is used
//   kDefaultString     - if enumFromString is used
//   kFallback/kFallbackTranslate - if enumToString is used (returned when value is not in table)
template <typename E>
struct EnumInfo;

template <>
struct EnumInfo<DNS_POLICY_TYPE>
{
    static constexpr EnumDesc<DNS_POLICY_TYPE> table[] = {
        { DNS_TYPE_OS_DEFAULT, QT_TRANSLATE_NOOP("QObject", "OS Default"), true },
        { DNS_TYPE_OPEN_DNS,   "OpenDNS",    false },
        { DNS_TYPE_CLOUDFLARE, "Cloudflare", false },
        { DNS_TYPE_GOOGLE,     "Google",     false },
        { DNS_TYPE_CONTROLD,   "Control D",  false },
    };
    static constexpr DNS_POLICY_TYPE kDefaultInt = DNS_TYPE_CLOUDFLARE;
    static constexpr DNS_POLICY_TYPE kDefaultString = DNS_TYPE_OS_DEFAULT;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<CONNECTED_DNS_TYPE>
{
    static constexpr EnumDesc<CONNECTED_DNS_TYPE> table[] = {
        { CONNECTED_DNS_TYPE_AUTO,     QT_TRANSLATE_NOOP("QObject", "Auto"),      true },
        { CONNECTED_DNS_TYPE_CUSTOM,   QT_TRANSLATE_NOOP("QObject", "Custom"),    true },
        { CONNECTED_DNS_TYPE_FORCED,   QT_TRANSLATE_NOOP("QObject", "Forced"),    true },
        { CONNECTED_DNS_TYPE_LOCAL,    QT_TRANSLATE_NOOP("QObject", "Local DNS"), true },
        { CONNECTED_DNS_TYPE_CONTROLD, "Control D", false },
    };
    static constexpr CONNECTED_DNS_TYPE kDefaultInt = CONNECTED_DNS_TYPE_AUTO;
    static constexpr CONNECTED_DNS_TYPE kDefaultString = CONNECTED_DNS_TYPE_AUTO;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<SPLIT_TUNNELING_MODE>
{
    static constexpr EnumDesc<SPLIT_TUNNELING_MODE> table[] = {
        { SPLIT_TUNNELING_MODE_EXCLUDE, QT_TRANSLATE_NOOP("QObject", "Exclude"), true },
        { SPLIT_TUNNELING_MODE_INCLUDE, QT_TRANSLATE_NOOP("QObject", "Include"), true },
    };
    static constexpr SPLIT_TUNNELING_MODE kDefaultInt = SPLIT_TUNNELING_MODE_EXCLUDE;
    static constexpr SPLIT_TUNNELING_MODE kDefaultString = SPLIT_TUNNELING_MODE_EXCLUDE;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

// Only an int conversion is needed for this enum.
template <>
struct EnumInfo<SPLIT_TUNNELING_NETWORK_ROUTE_TYPE>
{
    static constexpr EnumDesc<SPLIT_TUNNELING_NETWORK_ROUTE_TYPE> table[] = {
        { SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP,       "", false },
        { SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME, "", false },
    };
    static constexpr SPLIT_TUNNELING_NETWORK_ROUTE_TYPE kDefaultInt = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
};

// Only an int conversion is needed for this enum.
template <>
struct EnumInfo<SPLIT_TUNNELING_APP_TYPE>
{
    static constexpr EnumDesc<SPLIT_TUNNELING_APP_TYPE> table[] = {
        { SPLIT_TUNNELING_APP_TYPE_USER,   "", false },
        { SPLIT_TUNNELING_APP_TYPE_SYSTEM, "", false },
    };
    static constexpr SPLIT_TUNNELING_APP_TYPE kDefaultInt = SPLIT_TUNNELING_APP_TYPE_USER;
};

template <>
struct EnumInfo<PROXY_SHARING_TYPE>
{
    static constexpr EnumDesc<PROXY_SHARING_TYPE> table[] = {
        { PROXY_SHARING_HTTP,  "HTTP",  false },
        { PROXY_SHARING_SOCKS, "SOCKS", false },
    };
    static constexpr PROXY_SHARING_TYPE kDefaultInt = PROXY_SHARING_HTTP;
    static constexpr PROXY_SHARING_TYPE kDefaultString = PROXY_SHARING_HTTP;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<ORDER_LOCATION_TYPE>
{
    static constexpr EnumDesc<ORDER_LOCATION_TYPE> table[] = {
        { ORDER_LOCATION_BY_GEOGRAPHY,      QT_TRANSLATE_NOOP("QObject", "Geography"), true },
        { ORDER_LOCATION_BY_ALPHABETICALLY, QT_TRANSLATE_NOOP("QObject", "Alphabet"),  true },
        { ORDER_LOCATION_BY_LATENCY,        QT_TRANSLATE_NOOP("QObject", "Latency"),   true },
    };
    static constexpr ORDER_LOCATION_TYPE kDefaultInt = ORDER_LOCATION_BY_GEOGRAPHY;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<SERVER_ROUTING_METHOD_TYPE>
{
    static constexpr EnumDesc<SERVER_ROUTING_METHOD_TYPE> table[] = {
        { SERVER_ROUTING_METHOD_AUTO,        QT_TRANSLATE_NOOP("QObject", "Auto"),      true },
        { SERVER_ROUTING_METHOD_REGULAR,     QT_TRANSLATE_NOOP("QObject", "Regular"),   true },
        { SERVER_ROUTING_METHOD_ALTERNATIVE, QT_TRANSLATE_NOOP("QObject", "Alternate"), true },
    };
    static constexpr SERVER_ROUTING_METHOD_TYPE kDefaultInt = SERVER_ROUTING_METHOD_AUTO;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<PROTOCOL_TWEAKS_METHOD_TYPE>
{
    static constexpr EnumDesc<PROTOCOL_TWEAKS_METHOD_TYPE> table[] = {
        { PROTOCOL_TWEAKS_METHOD_AUTO,     QT_TRANSLATE_NOOP("QObject", "Auto"),     true },
        { PROTOCOL_TWEAKS_METHOD_ENABLED,  QT_TRANSLATE_NOOP("QObject", "Enabled"),  true },
        { PROTOCOL_TWEAKS_METHOD_DISABLED, QT_TRANSLATE_NOOP("QObject", "Disabled"), true },
    };
    static constexpr PROTOCOL_TWEAKS_METHOD_TYPE kDefaultInt = PROTOCOL_TWEAKS_METHOD_AUTO;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

// Only an int conversion is needed for this enum.
template <>
struct EnumInfo<BACKGROUND_TYPE>
{
    static constexpr EnumDesc<BACKGROUND_TYPE> table[] = {
        { BACKGROUND_TYPE_NONE,          "", false },
        { BACKGROUND_TYPE_COUNTRY_FLAGS, "", false },
        { BACKGROUND_TYPE_CUSTOM,        "", false },
        { BACKGROUND_TYPE_BUNDLED,       "", false },
    };
    static constexpr BACKGROUND_TYPE kDefaultInt = BACKGROUND_TYPE_NONE;
};

// Only an int conversion is needed for this enum.
template <>
struct EnumInfo<SOUND_NOTIFICATION_TYPE>
{
    static constexpr EnumDesc<SOUND_NOTIFICATION_TYPE> table[] = {
        { SOUND_NOTIFICATION_TYPE_NONE,    "", false },
        { SOUND_NOTIFICATION_TYPE_BUNDLED, "", false },
        { SOUND_NOTIFICATION_TYPE_CUSTOM,  "", false },
    };
    static constexpr SOUND_NOTIFICATION_TYPE kDefaultInt = SOUND_NOTIFICATION_TYPE_NONE;
};

// Only a string conversion is needed for this enum.
template <>
struct EnumInfo<TAP_ADAPTER_TYPE>
{
    static constexpr EnumDesc<TAP_ADAPTER_TYPE> table[] = {
        { DCO_ADAPTER, "DCO", false },
        { TAP_ADAPTER, "TAP", false },
    };
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<FIREWALL_MODE>
{
    static constexpr EnumDesc<FIREWALL_MODE> table[] = {
        { FIREWALL_MODE_MANUAL,         QT_TRANSLATE_NOOP("QObject", "Manual"),     true },
        { FIREWALL_MODE_AUTOMATIC,      QT_TRANSLATE_NOOP("QObject", "Auto"),       true },
        { FIREWALL_MODE_ALWAYS_ON,      QT_TRANSLATE_NOOP("QObject", "Always On"),  true },
        { FIREWALL_MODE_ALWAYS_ON_PLUS, QT_TRANSLATE_NOOP("QObject", "Always On+"), true },
    };
    static constexpr FIREWALL_MODE kDefaultInt = FIREWALL_MODE_AUTOMATIC;
    static constexpr FIREWALL_MODE kDefaultString = FIREWALL_MODE_AUTOMATIC;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<FIREWALL_WHEN>
{
    static constexpr EnumDesc<FIREWALL_WHEN> table[] = {
        { FIREWALL_WHEN_BEFORE_CONNECTION, QT_TRANSLATE_NOOP("QObject", "Before Connection"), true },
        { FIREWALL_WHEN_AFTER_CONNECTION,  QT_TRANSLATE_NOOP("QObject", "After Connection"),  true },
    };
    static constexpr FIREWALL_WHEN kDefaultInt = FIREWALL_WHEN_BEFORE_CONNECTION;
    static constexpr FIREWALL_WHEN kDefaultString = FIREWALL_WHEN_BEFORE_CONNECTION;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

// Only an int conversion is needed for this enum.  NETWORK_INTERFACE_MOBILE_BROADBAND (value 4) is
// intentionally omitted from the table so that fromInt() normalizes it to NONE, preserving the exact
// behavior of the original NETWORK_INTERFACE_TYPE_fromInt(), which only accepted 0..3.  Letting value
// 4 survive normalization is a deliberate behavior change tracked separately.
template <>
struct EnumInfo<NETWORK_INTERFACE_TYPE>
{
    static constexpr EnumDesc<NETWORK_INTERFACE_TYPE> table[] = {
        { NETWORK_INTERFACE_NONE, "", false },
        { NETWORK_INTERFACE_ETH,  "", false },
        { NETWORK_INTERFACE_WIFI, "", false },
        { NETWORK_INTERFACE_PPP,  "", false },
    };
    static constexpr NETWORK_INTERFACE_TYPE kDefaultInt = NETWORK_INTERFACE_NONE;
};

// NETWORK_TRUST_FORGET is a transitory state and is intentionally excluded from the table.  The
// to-string fallback returns the (untranslated) "Secured", matching the original behavior.
template <>
struct EnumInfo<NETWORK_TRUST_TYPE>
{
    static constexpr EnumDesc<NETWORK_TRUST_TYPE> table[] = {
        { NETWORK_TRUST_SECURED,   "Secured",   false },
        { NETWORK_TRUST_UNSECURED, "Unsecured", false },
    };
    static constexpr NETWORK_TRUST_TYPE kDefaultInt = NETWORK_TRUST_SECURED;
    static constexpr NETWORK_TRUST_TYPE kDefaultString = NETWORK_TRUST_SECURED;
    static constexpr const char *kFallback = "Secured";
    static constexpr bool kFallbackTranslate = false;
};

template <>
struct EnumInfo<PROXY_OPTION>
{
    static constexpr EnumDesc<PROXY_OPTION> table[] = {
        { PROXY_OPTION_NONE,       QT_TRANSLATE_NOOP("QObject", "None"),        true },
        { PROXY_OPTION_AUTODETECT, QT_TRANSLATE_NOOP("QObject", "Auto-detect"), true },
        { PROXY_OPTION_HTTP,       "HTTP",  false },
        { PROXY_OPTION_SOCKS,      "SOCKS", false },
    };
    static constexpr PROXY_OPTION kDefaultInt = PROXY_OPTION_NONE;
    static constexpr PROXY_OPTION kDefaultString = PROXY_OPTION_NONE;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<UPDATE_CHANNEL>
{
    static constexpr EnumDesc<UPDATE_CHANNEL> table[] = {
        { UPDATE_CHANNEL_RELEASE,    QT_TRANSLATE_NOOP("QObject", "Release"),    true },
        { UPDATE_CHANNEL_BETA,       QT_TRANSLATE_NOOP("QObject", "Beta"),       true },
        { UPDATE_CHANNEL_GUINEA_PIG, QT_TRANSLATE_NOOP("QObject", "Guinea Pig"), true },
        { UPDATE_CHANNEL_INTERNAL,   QT_TRANSLATE_NOOP("QObject", "Internal"),   true },
    };
    static constexpr UPDATE_CHANNEL kDefaultInt = UPDATE_CHANNEL_RELEASE;
    static constexpr UPDATE_CHANNEL kDefaultString = UPDATE_CHANNEL_RELEASE;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<DNS_MANAGER_TYPE>
{
    static constexpr EnumDesc<DNS_MANAGER_TYPE> table[] = {
        { DNS_MANAGER_AUTOMATIC,        QT_TRANSLATE_NOOP("QObject", "Auto"), true },
        { DNS_MANAGER_RESOLV_CONF,      "resolvconf",       false },
        { DNS_MANAGER_SYSTEMD_RESOLVED, "systemd-resolved", false },
        { DNS_MANAGER_NETWORK_MANAGER,  "NetworkManager",   false },
    };
    static constexpr DNS_MANAGER_TYPE kDefaultInt = DNS_MANAGER_AUTOMATIC;
    static constexpr DNS_MANAGER_TYPE kDefaultString = DNS_MANAGER_AUTOMATIC;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<APP_SKIN>
{
    static constexpr EnumDesc<APP_SKIN> table[] = {
        { APP_SKIN_ALPHA,    QT_TRANSLATE_NOOP("QObject", "Alpha"),    true },
        { APP_SKIN_VAN_GOGH, QT_TRANSLATE_NOOP("QObject", "Van Gogh"), true },
    };
    static constexpr APP_SKIN kDefaultInt = APP_SKIN_ALPHA;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

// TOGGLE_MODE strings are intentionally not translated (returned verbatim), matching the original.
template <>
struct EnumInfo<TOGGLE_MODE>
{
    static constexpr EnumDesc<TOGGLE_MODE> table[] = {
        { TOGGLE_MODE_AUTO,   "Auto",   false },
        { TOGGLE_MODE_MANUAL, "Manual", false },
    };
    static constexpr TOGGLE_MODE kDefaultString = TOGGLE_MODE_AUTO;
    static constexpr const char *kFallback = "Auto";
    static constexpr bool kFallbackTranslate = false;
};

template <>
struct EnumInfo<MULTI_DESKTOP_BEHAVIOR>
{
    static constexpr EnumDesc<MULTI_DESKTOP_BEHAVIOR> table[] = {
        { MULTI_DESKTOP_AUTO,        QT_TRANSLATE_NOOP("QObject", "Auto"),        true },
        { MULTI_DESKTOP_DUPLICATE,   QT_TRANSLATE_NOOP("QObject", "Duplicate"),   true },
        { MULTI_DESKTOP_MOVE_SPACES, QT_TRANSLATE_NOOP("QObject", "Move spaces"), true },
        { MULTI_DESKTOP_MOVE_WINDOW, QT_TRANSLATE_NOOP("QObject", "Move window"), true },
    };
    static constexpr MULTI_DESKTOP_BEHAVIOR kDefaultInt = MULTI_DESKTOP_AUTO;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "Auto");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<ASPECT_RATIO_MODE>
{
    static constexpr EnumDesc<ASPECT_RATIO_MODE> table[] = {
        { ASPECT_RATIO_MODE_STRETCH, QT_TRANSLATE_NOOP("QObject", "Stretch"), true },
        { ASPECT_RATIO_MODE_FILL,    QT_TRANSLATE_NOOP("QObject", "Fill"),    true },
        { ASPECT_RATIO_MODE_TILE,    QT_TRANSLATE_NOOP("QObject", "Tile"),    true },
    };
    static constexpr ASPECT_RATIO_MODE kDefaultInt = ASPECT_RATIO_MODE_STRETCH;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

template <>
struct EnumInfo<IpStack>
{
    static constexpr EnumDesc<IpStack> table[] = {
        { IpStack::kAuto,     QT_TRANSLATE_NOOP("QObject", "Auto"),      true },
        { IpStack::kIPv4Only, QT_TRANSLATE_NOOP("QObject", "IPv4 Only"), true },
    };
    static constexpr IpStack kDefaultInt = IpStack::kAuto;
    static constexpr IpStack kDefaultString = IpStack::kAuto;
    static constexpr const char *kFallback = QT_TRANSLATE_NOOP("QObject", "UNKNOWN");
    static constexpr bool kFallbackTranslate = true;
};

} // anonymous namespace


template <typename E>
E enumFromInt(int t)
{
    for (const auto &e : EnumInfo<E>::table) {
        if (static_cast<int>(e.value) == t)
            return e.value;
    }
    return EnumInfo<E>::kDefaultInt;
}

template <typename E>
E enumFromString(const QString &s)
{
    for (const auto &e : EnumInfo<E>::table) {
        if (s == QLatin1String(e.str))
            return e.value;
    }
    WS_ASSERT(false);
    return EnumInfo<E>::kDefaultString;
}

template <typename E>
QString enumToString(E value)
{
    for (const auto &e : EnumInfo<E>::table) {
        if (e.value == value)
            return e.translate ? QObject::tr(e.str) : QString::fromLatin1(e.str);
    }
    WS_ASSERT(false);
    return EnumInfo<E>::kFallbackTranslate ? QObject::tr(EnumInfo<E>::kFallback)
                                           : QString::fromLatin1(EnumInfo<E>::kFallback);
}

template <typename E>
QList<QPair<QString, QVariant>> enumToList()
{
    QList<QPair<QString, QVariant>> l;
    for (const auto &e : EnumInfo<E>::table)
        l.append(qMakePair(enumToString<E>(e.value), QVariant(static_cast<int>(e.value))));
    return l;
}


// TRAY_ICON_COLOR is platform-dependent (default value, available options and OS-theme handling all
// differ per OS), so it keeps its hand-written implementation rather than using the generic tables.
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


// Explicit instantiations: exactly the enum/operation combinations used across the codebase.
template DNS_POLICY_TYPE enumFromInt<DNS_POLICY_TYPE>(int);
template DNS_POLICY_TYPE enumFromString<DNS_POLICY_TYPE>(const QString &);
template QString enumToString<DNS_POLICY_TYPE>(DNS_POLICY_TYPE);
template QList<QPair<QString, QVariant>> enumToList<DNS_POLICY_TYPE>();

template CONNECTED_DNS_TYPE enumFromInt<CONNECTED_DNS_TYPE>(int);
template CONNECTED_DNS_TYPE enumFromString<CONNECTED_DNS_TYPE>(const QString &);
template QString enumToString<CONNECTED_DNS_TYPE>(CONNECTED_DNS_TYPE);

template SPLIT_TUNNELING_MODE enumFromInt<SPLIT_TUNNELING_MODE>(int);
template SPLIT_TUNNELING_MODE enumFromString<SPLIT_TUNNELING_MODE>(const QString &);
template QString enumToString<SPLIT_TUNNELING_MODE>(SPLIT_TUNNELING_MODE);

template SPLIT_TUNNELING_NETWORK_ROUTE_TYPE enumFromInt<SPLIT_TUNNELING_NETWORK_ROUTE_TYPE>(int);
template SPLIT_TUNNELING_APP_TYPE enumFromInt<SPLIT_TUNNELING_APP_TYPE>(int);

template PROXY_SHARING_TYPE enumFromInt<PROXY_SHARING_TYPE>(int);
template PROXY_SHARING_TYPE enumFromString<PROXY_SHARING_TYPE>(const QString &);
template QString enumToString<PROXY_SHARING_TYPE>(PROXY_SHARING_TYPE);
template QList<QPair<QString, QVariant>> enumToList<PROXY_SHARING_TYPE>();

template ORDER_LOCATION_TYPE enumFromInt<ORDER_LOCATION_TYPE>(int);
template QString enumToString<ORDER_LOCATION_TYPE>(ORDER_LOCATION_TYPE);
template QList<QPair<QString, QVariant>> enumToList<ORDER_LOCATION_TYPE>();

template SERVER_ROUTING_METHOD_TYPE enumFromInt<SERVER_ROUTING_METHOD_TYPE>(int);
template QString enumToString<SERVER_ROUTING_METHOD_TYPE>(SERVER_ROUTING_METHOD_TYPE);
template QList<QPair<QString, QVariant>> enumToList<SERVER_ROUTING_METHOD_TYPE>();

template PROTOCOL_TWEAKS_METHOD_TYPE enumFromInt<PROTOCOL_TWEAKS_METHOD_TYPE>(int);
template QString enumToString<PROTOCOL_TWEAKS_METHOD_TYPE>(PROTOCOL_TWEAKS_METHOD_TYPE);
template QList<QPair<QString, QVariant>> enumToList<PROTOCOL_TWEAKS_METHOD_TYPE>();

template BACKGROUND_TYPE enumFromInt<BACKGROUND_TYPE>(int);
template SOUND_NOTIFICATION_TYPE enumFromInt<SOUND_NOTIFICATION_TYPE>(int);

template QString enumToString<TAP_ADAPTER_TYPE>(TAP_ADAPTER_TYPE);

template FIREWALL_MODE enumFromInt<FIREWALL_MODE>(int);
template FIREWALL_MODE enumFromString<FIREWALL_MODE>(const QString &);
template QString enumToString<FIREWALL_MODE>(FIREWALL_MODE);
template QList<QPair<QString, QVariant>> enumToList<FIREWALL_MODE>();

template FIREWALL_WHEN enumFromInt<FIREWALL_WHEN>(int);
template FIREWALL_WHEN enumFromString<FIREWALL_WHEN>(const QString &);
template QString enumToString<FIREWALL_WHEN>(FIREWALL_WHEN);
template QList<QPair<QString, QVariant>> enumToList<FIREWALL_WHEN>();

template NETWORK_INTERFACE_TYPE enumFromInt<NETWORK_INTERFACE_TYPE>(int);

template NETWORK_TRUST_TYPE enumFromInt<NETWORK_TRUST_TYPE>(int);
template NETWORK_TRUST_TYPE enumFromString<NETWORK_TRUST_TYPE>(const QString &);
template QString enumToString<NETWORK_TRUST_TYPE>(NETWORK_TRUST_TYPE);

template PROXY_OPTION enumFromInt<PROXY_OPTION>(int);
template PROXY_OPTION enumFromString<PROXY_OPTION>(const QString &);
template QString enumToString<PROXY_OPTION>(PROXY_OPTION);
template QList<QPair<QString, QVariant>> enumToList<PROXY_OPTION>();

template UPDATE_CHANNEL enumFromInt<UPDATE_CHANNEL>(int);
template UPDATE_CHANNEL enumFromString<UPDATE_CHANNEL>(const QString &);
template QString enumToString<UPDATE_CHANNEL>(UPDATE_CHANNEL);
template QList<QPair<QString, QVariant>> enumToList<UPDATE_CHANNEL>();

template DNS_MANAGER_TYPE enumFromInt<DNS_MANAGER_TYPE>(int);
template DNS_MANAGER_TYPE enumFromString<DNS_MANAGER_TYPE>(const QString &);
template QString enumToString<DNS_MANAGER_TYPE>(DNS_MANAGER_TYPE);
template QList<QPair<QString, QVariant>> enumToList<DNS_MANAGER_TYPE>();

template APP_SKIN enumFromInt<APP_SKIN>(int);
template QString enumToString<APP_SKIN>(APP_SKIN);
template QList<QPair<QString, QVariant>> enumToList<APP_SKIN>();

template TOGGLE_MODE enumFromString<TOGGLE_MODE>(const QString &);
template QString enumToString<TOGGLE_MODE>(TOGGLE_MODE);

template MULTI_DESKTOP_BEHAVIOR enumFromInt<MULTI_DESKTOP_BEHAVIOR>(int);
template QString enumToString<MULTI_DESKTOP_BEHAVIOR>(MULTI_DESKTOP_BEHAVIOR);
template QList<QPair<QString, QVariant>> enumToList<MULTI_DESKTOP_BEHAVIOR>();

template ASPECT_RATIO_MODE enumFromInt<ASPECT_RATIO_MODE>(int);
template QString enumToString<ASPECT_RATIO_MODE>(ASPECT_RATIO_MODE);
template QList<QPair<QString, QVariant>> enumToList<ASPECT_RATIO_MODE>();

template IpStack enumFromInt<IpStack>(int);
template IpStack enumFromString<IpStack>(const QString &);
template QString enumToString<IpStack>(IpStack);
template QList<QPair<QString, QVariant>> enumToList<IpStack>();
