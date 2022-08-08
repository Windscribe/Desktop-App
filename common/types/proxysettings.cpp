#include "proxysettings.h"
#include "utils/logger.h"

const int typeIdProxySettings = qRegisterMetaType<types::ProxySettings>("types::ProxySettings");

namespace types {

ProxySettings::ProxySettings(): option_(PROXY_OPTION_NONE), port_(0)
{
}

ProxySettings::ProxySettings(PROXY_OPTION option, const QString &address, uint port, const QString &password, const QString &username)
{
    option_ = option;
    address_ = address;
    port_ = port;
    password_ = password;
    username_ = username;
}

PROXY_OPTION ProxySettings::option() const
{
    return option_;
}

void ProxySettings::setOption(PROXY_OPTION option)
{
    option_ = option;
}

QString ProxySettings::address() const
{
    return address_;
}

void ProxySettings::setAddress(const QString &address)
{
    address_ = address;
}

uint ProxySettings::getPort() const
{
    return port_;
}

void ProxySettings::setPort(const uint &value)
{
    port_ = value;
}

QString ProxySettings::getUsername() const
{
    return username_;
}

void ProxySettings::setUsername(const QString &username)
{
    username_ = username;
}

QString ProxySettings::getPassword() const
{
    return password_;
}

void ProxySettings::setPassword(const QString &password)
{
    password_ = password;
}

QNetworkProxy ProxySettings::getNetworkProxy() const
{
    QNetworkProxy proxy;
    if (option_ == PROXY_OPTION_NONE)
    {
        proxy.setType(QNetworkProxy::NoProxy);
        proxy.setHostName(address_);
        proxy.setPort(port_);
        proxy.setUser(username_);
        proxy.setPassword(password_);
    }
    else if (option_ == PROXY_OPTION_AUTODETECT)
    {
        Q_ASSERT(false);
    }
    else if (option_ == PROXY_OPTION_HTTP)
    {
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(address_);
        proxy.setPort(port_);
        proxy.setUser(username_);
        proxy.setPassword(password_);
    }
    else if (option_ == PROXY_OPTION_SOCKS)
    {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(address_);
        proxy.setPort(port_);
        proxy.setUser(username_);
        proxy.setPassword(password_);
    }
    else
    {
        Q_ASSERT(false);
    }
    return proxy;
}

bool ProxySettings::isProxyEnabled() const
{
    return (option_ == PROXY_OPTION_HTTP || option_ == PROXY_OPTION_SOCKS);
}

QJsonObject ProxySettings::toJsonObject() const
{
    QJsonObject json;
    json["option"] = (int)option_;
    json["address"] = address_;
    json["port"] = (qint64)port_;
    json["username"] = username_;
    json["password"] = password_;
    return json;
}

bool ProxySettings::fromJsonObject(const QJsonObject &json)
{
    if (json.contains("option")) option_ = (PROXY_OPTION)json["option"].toInt(PROXY_OPTION_NONE);
    if (json.contains("address")) address_ = json["address"].toString();
    if (json.contains("port")) port_ = json["port"].toInteger(0);
    if (json.contains("username")) username_ = json["username"].toString();
    if (json.contains("password")) password_ = json["password"].toString();
    return true;
}

QDebug operator<<(QDebug dbg, const ProxySettings &ps)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{option:" << PROXY_OPTION_toString(ps.option_) << "; ";
    dbg << "address:" << ps.address_ << "; ";
    dbg << "port:" << ps.port_ << "; ";
    dbg << "username:" << (ps.username_.isEmpty() ? "empty" : "settled") << "; ";
    dbg << "password:" << (ps.password_.isEmpty() ? "empty" : "settled") << "}";
    return dbg;
}

} //namespace types
