#include "enums.h"
#include <QList>
#include <QMetaType>

const int typeIdOpenVpnError = qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
const int typeIdProtocol = qRegisterMetaType<PROTOCOL>("PROTOCOL");
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
    if (ret == LOGIN_RET_SUCCESS)
    {
        return "SUCCESS";
    }
    else if (ret == LOGIN_RET_NO_API_CONNECTIVITY)
    {
        return "NO_API_CONNECTIVITY";
    }
    else if (ret == LOGIN_RET_NO_CONNECTIVITY)
    {
        return "NO_CONNECTIVITY";
    }
    else if (ret == LOGIN_RET_INCORRECT_JSON)
    {
        return "INCORRECT_JSON";
    }
    else if (ret == LOGIN_RET_BAD_USERNAME)
    {
        return "BAD_USERNAME";
    }
    else if (ret == LOGIN_RET_PROXY_AUTH_NEED)
    {
        return "PROXY_AUTH_NEED";
    }
    else if (ret == LOGIN_RET_SSL_ERROR)
    {
        return "SSL_ERROR";
    }
    else if (ret == LOGIN_RET_BAD_CODE2FA)
    {
        return "BAD_CODE2FA";
    }
    else if (ret == LOGIN_RET_MISSING_CODE2FA)
    {
        return "MISSING_CODE2FA";
    }
    else if (ret == LOGIN_RET_ACCOUNT_DISABLED)
    {
        return "ACCOUNT_DISABLED";
    }
    else if (ret == LOGIN_RET_SESSION_INVALID)
    {
        return "SESSION_INVALID";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString DNS_POLICY_TYPE_ToString(DNS_POLICY_TYPE d)
{
    if (d == DNS_TYPE_OS_DEFAULT)
    {
        return "OS Default";
    }
    else if (d == DNS_TYPE_OPEN_DNS)
    {
        return "OpenDNS";
    }
    else if (d == DNS_TYPE_CLOUDFLARE)
    {
        return "Cloudflare";
    }
    else if (d == DNS_TYPE_GOOGLE)
    {
        return "Google";
    }
    else if (d == DNS_TYPE_CONTROLD)
    {
        return "ControlD";
    }
    else
    {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString PROXY_SHARING_TYPE_toString(PROXY_SHARING_TYPE t)
{
    if (t == PROXY_SHARING_HTTP) return "HTTP";
    else if (t == PROXY_SHARING_SOCKS) return "SOCKS";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_TYPE p)
{
    if (p == ORDER_LOCATION_BY_GEOGRAPHY) return "Geography";
    else if (p == ORDER_LOCATION_BY_ALPHABETICALLY) return "Alphabet";
    else if (p == ORDER_LOCATION_BY_LATENCY) return "Latency";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_TYPE t)
{
    if (t == LATENCY_DISPLAY_BARS) return "Bars";
    else if (t == LATENCY_DISPLAY_MS) return "Ms";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString TAP_ADAPTER_TYPE_toString(TAP_ADAPTER_TYPE t)
{
    if (t == TAP_ADAPTER) return "Windscribe VPN";
    else if (t == WINTUN_ADAPTER) return "Wintun";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QString FIREWALL_MODE_toString(FIREWALL_MODE t)
{
    if (t == FIREWALL_MODE_MANUAL) return "Manual";
    else if (t == FIREWALL_MODE_AUTOMATIC) return "Automatic";
    else if (t == FIREWALL_MODE_ALWAYS_ON) return "Always On";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QList<QPair<QString, int> > FIREWALL_MODE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_MANUAL), FIREWALL_MODE_MANUAL);
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_AUTOMATIC), FIREWALL_MODE_AUTOMATIC);
    l << qMakePair(FIREWALL_MODE_toString(FIREWALL_MODE_ALWAYS_ON), FIREWALL_MODE_ALWAYS_ON);
    return l;
}

QString FIREWALL_WHEN_toString(FIREWALL_WHEN t)
{
    if (t == FIREWALL_WHEN_BEFORE_CONNECTION) return "Before Connection";
    else if (t == FIREWALL_WHEN_AFTER_CONNECTION) return "After Connection";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QList<QPair<QString, int> > FIREWALL_WHEN_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(FIREWALL_WHEN_toString(FIREWALL_WHEN_BEFORE_CONNECTION), FIREWALL_WHEN_BEFORE_CONNECTION);
    l << qMakePair(FIREWALL_WHEN_toString(FIREWALL_WHEN_AFTER_CONNECTION), FIREWALL_WHEN_AFTER_CONNECTION);
    return l;
}

QList<QPair<QString, int> > PROXY_SHARING_TYPE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(PROXY_SHARING_TYPE_toString(PROXY_SHARING_HTTP), PROXY_SHARING_HTTP);
    l << qMakePair(PROXY_SHARING_TYPE_toString(PROXY_SHARING_SOCKS), PROXY_SHARING_SOCKS);
    return l;
}

QString PROXY_OPTION_toString(PROXY_OPTION t)
{
    if (t == PROXY_OPTION_NONE) return "None";
    else if (t == PROXY_OPTION_AUTODETECT) return "Auto-detect";
    else if (t == PROXY_OPTION_HTTP) return "HTTP";
    else if (t == PROXY_OPTION_SOCKS) return "SOCKS";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QList<QPair<QString, int> > PROXY_OPTION_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_NONE), PROXY_OPTION_NONE);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_AUTODETECT), PROXY_OPTION_AUTODETECT);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_HTTP), PROXY_OPTION_HTTP);
    l << qMakePair(PROXY_OPTION_toString(PROXY_OPTION_SOCKS), PROXY_OPTION_SOCKS);
    return l;
}

QList<QPair<QString, int> > DNS_POLICY_TYPE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(DNS_POLICY_TYPE_ToString(DNS_TYPE_OS_DEFAULT), DNS_TYPE_OS_DEFAULT);
    l << qMakePair(DNS_POLICY_TYPE_ToString(DNS_TYPE_OPEN_DNS), DNS_TYPE_OPEN_DNS);
    l << qMakePair(DNS_POLICY_TYPE_ToString(DNS_TYPE_CLOUDFLARE), DNS_TYPE_CLOUDFLARE);
    l << qMakePair(DNS_POLICY_TYPE_ToString(DNS_TYPE_GOOGLE), DNS_TYPE_GOOGLE);
    l << qMakePair(DNS_POLICY_TYPE_ToString(DNS_TYPE_CONTROLD), DNS_TYPE_CONTROLD);
    return l;
}

QList<QPair<QString, int> > ORDER_LOCATION_TYPE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_GEOGRAPHY), ORDER_LOCATION_BY_GEOGRAPHY);
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_ALPHABETICALLY), ORDER_LOCATION_BY_ALPHABETICALLY);
    l << qMakePair(ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_BY_LATENCY), ORDER_LOCATION_BY_LATENCY);
    return l;

}

QList<QPair<QString, int> > LATENCY_DISPLAY_TYPE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_BARS), LATENCY_DISPLAY_BARS);
    l << qMakePair(LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_MS), LATENCY_DISPLAY_MS);
    return l;
}

QString UPDATE_CHANNEL_toString(UPDATE_CHANNEL t)
{
    if (t == UPDATE_CHANNEL_RELEASE) return "Release";
    else if (t == UPDATE_CHANNEL_BETA) return "Beta";
    else if (t == UPDATE_CHANNEL_GUINEA_PIG) return "Guinea Pig";
    else if (t == UPDATE_CHANNEL_INTERNAL) return "Internal";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QList<QPair<QString, int> > UPDATE_CHANNEL_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_RELEASE), UPDATE_CHANNEL_RELEASE);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_BETA), UPDATE_CHANNEL_BETA);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_GUINEA_PIG), UPDATE_CHANNEL_GUINEA_PIG);
    l << qMakePair(UPDATE_CHANNEL_toString(UPDATE_CHANNEL_INTERNAL), UPDATE_CHANNEL_INTERNAL);
    return l;
}

QString DNS_MANAGER_TYPE_toString(DNS_MANAGER_TYPE t)
{
    if (t == DNS_MANAGER_AUTOMATIC) return "Automatic";
    else if (t == DNS_MANAGER_RESOLV_CONF) return "Resolvconf";
    else if (t == DNS_MANAGER_SYSTEMD_RESOLVED) return "Systemd-resolved";
    else if (t == DNS_MANAGER_NETWORK_MANAGER) return "NetworkManager";
    else {
        Q_ASSERT(false);
        return "UNKNOWN";
    }
}

QList<QPair<QString, int> > DNS_MANAGER_TYPE_toList()
{
    QList<QPair<QString, int> > l;
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_AUTOMATIC), DNS_MANAGER_AUTOMATIC);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_RESOLV_CONF), DNS_MANAGER_RESOLV_CONF);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_SYSTEMD_RESOLVED), DNS_MANAGER_SYSTEMD_RESOLVED);
    l << qMakePair(DNS_MANAGER_TYPE_toString(DNS_MANAGER_NETWORK_MANAGER), DNS_MANAGER_NETWORK_MANAGER);
    return l;

}

QString PROTOCOL::toShortString() const
{
    if (value_ == OPENVPN_UDP || value_ == OPENVPN_TCP || value_ == STUNNEL
        || value_ == WSTUNNEL) {
        return "openvpn";
    }
    else if (value_ == IKEV2) {
        return "ikev2";
    }
    else if (value_ == WIREGUARD) {
        return "wireguard";
    }
    else {
        Q_ASSERT(false);
        return "unknown";
    }
}

QString PROTOCOL::toLongString() const
{
    if (value_ == IKEV2) {
        return "IKEv2";
    }
    else if (value_ == OPENVPN_UDP)  {
        return "UDP";
    }
    else if (value_ == OPENVPN_TCP)  {
        return "TCP";
    }
    else if (value_ == STUNNEL)  {
        return "Stealth";
    }
    else if (value_ == WSTUNNEL) {
        return "WStunnel";
    }
    else if (value_ == WIREGUARD)  {
        return "WireGuard";
    }
    else
    {
        Q_ASSERT(false);
        return "unknown";
    }
}

bool PROTOCOL::isOpenVpnProtocol() const
{
    return value_ == OPENVPN_UDP || value_ == OPENVPN_TCP ||
            value_ == STUNNEL || value_ == WSTUNNEL;
}

bool PROTOCOL::isStunnelOrWStunnelProtocol() const
{
    return value_ == STUNNEL || value_ == WSTUNNEL;
}

bool PROTOCOL::isIkev2Protocol() const
{
    return value_ == IKEV2;
}

bool PROTOCOL::isWireGuardProtocol() const
{
    return value_ == WIREGUARD;
}

PROTOCOL PROTOCOL::fromString(const QString &strProtocol)
{
    if (strProtocol.compare("UDP", Qt::CaseInsensitive) == 0) {
        return OPENVPN_UDP;
    }
    else if (strProtocol.compare("TCP", Qt::CaseInsensitive) == 0) {
        return OPENVPN_TCP;
    }
    else if (strProtocol.compare("Stealth", Qt::CaseInsensitive) == 0) {
        return STUNNEL;
    }
    else if (strProtocol.compare("WStunnel", Qt::CaseInsensitive) == 0) {
        return WSTUNNEL;
    }
    else if (strProtocol.compare("WireGuard", Qt::CaseInsensitive) == 0) {
        return WIREGUARD;
    }
    else if (strProtocol.compare("IKEv2", Qt::CaseInsensitive) == 0) {
        return IKEV2;
    }
    else
    {
        return UNINITIALIZED;
    }
}

QDataStream& operator <<(QDataStream &stream, const PROTOCOL &o)
{
    stream << o.toInt();
    return stream;
}

QDataStream& operator >>(QDataStream &stream, PROTOCOL &o)
{
    int v;
    stream >> v;
    o = (PROTOCOL)v;
    return stream;
}

QString CONNECTED_DNS_TYPE_toString(CONNECTED_DNS_TYPE t)
{
    if (t == CONNECTED_DNS_TYPE_ROBERT) {
        return "R.O.B.E.R.T.";
    }
    else if (t == CONNECTED_DNS_TYPE_CUSTOM) {
        return "Custom";
    }
    else {
        Q_ASSERT(false);
        return "unknown";
    }
}

QString SPLIT_TUNNELING_MODE_toString(SPLIT_TUNNELING_MODE t)
{
    if (t == SPLIT_TUNNELING_MODE_EXCLUDE) {
        return "Exclude";
    }
    else if (t == SPLIT_TUNNELING_MODE_INCLUDE) {
        return "Include";
    }
    else {
        Q_ASSERT(false);
        return "unknown";
    }
}
