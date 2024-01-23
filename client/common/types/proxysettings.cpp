#include "proxysettings.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"

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

ProxySettings::ProxySettings(const QJsonObject &json)
{
    if (json.contains(jsonInfo_.kOptionProp) && json[jsonInfo_.kOptionProp].isDouble())
        option_ = static_cast<PROXY_OPTION>(json[jsonInfo_.kOptionProp].toInt());

    if (json.contains(jsonInfo_.kAddressProp) && json[jsonInfo_.kAddressProp].isString())
        address_ = json[jsonInfo_.kAddressProp].toString();

    if (json.contains(jsonInfo_.kPortProp) && json[jsonInfo_.kPortProp].isDouble())
        port_ = static_cast<uint>(json[jsonInfo_.kPortProp].toInt());

    if (json.contains(jsonInfo_.kUsernameProp) && json[jsonInfo_.kUsernameProp].isString())
        username_ = json[jsonInfo_.kUsernameProp].toString();

    if (json.contains(jsonInfo_.kPasswordProp) && json[jsonInfo_.kPasswordProp].isString())
        password_ = Utils::fromBase64(json[jsonInfo_.kPasswordProp].toString());
}

QJsonObject ProxySettings::toJson() const
{
    QJsonObject json;
    json[jsonInfo_.kOptionProp] = static_cast<int>(option_);
    json[jsonInfo_.kAddressProp] = address_;
    json[jsonInfo_.kPortProp] = static_cast<int>(port_);
    json[jsonInfo_.kUsernameProp] = username_;
    json[jsonInfo_.kPasswordProp] = Utils::toBase64(password_);
    return json;
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
        WS_ASSERT(false);
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
        WS_ASSERT(false);
    }
    return proxy;
}

bool ProxySettings::isProxyEnabled() const
{
    return (option_ == PROXY_OPTION_HTTP || option_ == PROXY_OPTION_SOCKS);
}

QDataStream& operator <<(QDataStream &stream, const ProxySettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.option_ << o.address_ << o.port_ << o.username_ << o.password_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, ProxySettings &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.option_ >> o.address_ >> o.port_ >> o.username_ >> o.password_;
    return stream;
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
