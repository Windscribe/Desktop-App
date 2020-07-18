#include "proxysettings.h"
#include "Utils/logger.h"

const int typeIdProxySettings = qRegisterMetaType<ProxySettings>("ProxySettings");

ProxySettings::ProxySettings(): option_(PROXY_OPTION_NONE)
{
}

ProxySettings::ProxySettings(const ProtoTypes::ProxySettings &p)
{
    if (p.proxy_option() == ProtoTypes::PROXY_OPTION_NONE)
    {
        option_ = PROXY_OPTION_NONE;
    }
    else if (p.proxy_option() == ProtoTypes::PROXY_OPTION_AUTODETECT)
    {
        option_ = PROXY_OPTION_AUTODETECT;
    }
    else if (p.proxy_option() == ProtoTypes::PROXY_OPTION_HTTP)
    {
        option_ = PROXY_OPTION_HTTP;
    }
    else if (p.proxy_option() == ProtoTypes::PROXY_OPTION_SOCKS)
    {
        option_ = PROXY_OPTION_SOCKS;
    }
    else
    {
        Q_ASSERT(false);
    }
    address_ = QString::fromStdString(p.address());
    port_ = p.port();
    username_ = QString::fromStdString(p.username());
    password_ = QString::fromStdString(p.password());
}

void ProxySettings::readFromSettings(QSettings &settings)
{
    option_ = (PROXY_OPTION)settings.value("proxyOption", PROXY_OPTION_NONE).toInt();
    address_ = settings.value("proxyAddress", "").toString();
    port_ = settings.value("proxyPort", "").toUInt();
    username_ = settings.value("proxyUsername", "").toString();
    password_ = settings.value("proxyPassword", "").toString();
}

void ProxySettings::writeToSettings(QSettings &settings)
{
    settings.setValue("proxyOption", (int)option_);
    settings.setValue("proxyAddress", address_);
    settings.setValue("proxyPort", port_);
    settings.setValue("proxyUsername", username_);
    settings.setValue("proxyPassword", password_);
}

bool ProxySettings::isEqual(const ProxySettings &other) const
{
    return option_ == other.option_ &&
           address_ == other.address_ &&
           port_ == other.port_ &&
           username_ == other.username_ &&
           password_ == other.password_;
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

void ProxySettings::debugToLog()
{
    switch (option_)
    {
        case PROXY_OPTION_NONE:
            qCDebug(LOG_BASIC) << "Proxy option: none";
            break;
        case PROXY_OPTION_AUTODETECT:
            qCDebug(LOG_BASIC) << "Proxy option: autodetect";
            break;
        case PROXY_OPTION_HTTP:
            qCDebug(LOG_BASIC) << "Proxy option: https";
            break;
        case PROXY_OPTION_SOCKS:
            qCDebug(LOG_BASIC) << "Proxy option: socks";
            break;
        default:
            Q_ASSERT(false);
    }

    qCDebug(LOG_BASIC) << "Proxy address:" << address_;
    qCDebug(LOG_BASIC) << "Proxy port:" << port_;
    if (!username_.isEmpty())
        qCDebug(LOG_BASIC) << "Proxy username: setted";
    else
        qCDebug(LOG_BASIC) << "Proxy username: empty";

    if (!password_.isEmpty())
        qCDebug(LOG_BASIC) << "Proxy password: setted";
    else
        qCDebug(LOG_BASIC) << "Proxy password: empty";
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

ProtoTypes::ProxySettings ProxySettings::convertToProtobuf() const
{
    ProtoTypes::ProxySettings ps;

    if (option_ == PROXY_OPTION_NONE)
    {
        ps.set_proxy_option(ProtoTypes::PROXY_OPTION_NONE);
    }
    else if (option_ == PROXY_OPTION_AUTODETECT)
    {
        ps.set_proxy_option(ProtoTypes::PROXY_OPTION_AUTODETECT);
    }
    else if (option_ == PROXY_OPTION_HTTP)
    {
        ps.set_proxy_option(ProtoTypes::PROXY_OPTION_HTTP);
    }
    else if (option_ == PROXY_OPTION_SOCKS)
    {
        ps.set_proxy_option(ProtoTypes::PROXY_OPTION_SOCKS);
    }
    else
    {
        Q_ASSERT(false);
    }

    ps.set_address(address_.toStdString());
    ps.set_port(port_);
    ps.set_username(username_.toStdString());
    ps.set_password(password_.toStdString());
    return ps;
}
