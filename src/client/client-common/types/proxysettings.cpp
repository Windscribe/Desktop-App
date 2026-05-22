#include "proxysettings.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

const int typeIdProxySettings = qRegisterMetaType<types::ProxySettings>("types::ProxySettings");

namespace types {

ProxySettings::ProxySettings()
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

ProxySettings::ProxySettings(const QJsonObject &json) : option_(PROXY_OPTION_NONE), port_(0)
{
    if (json.contains(kJsonOptionProp) && json[kJsonOptionProp].isDouble()) {
        option_ = PROXY_OPTION_fromInt(json[kJsonOptionProp].toInt());
    }

    if (json.contains(kJsonAddressProp) && json[kJsonAddressProp].isString()) {
        address_ = json[kJsonAddressProp].toString();
    }

    if (json.contains(kJsonPortProp) && json[kJsonPortProp].isDouble()) {
        port_ = static_cast<uint>(json[kJsonPortProp].toInt());
    }

    if (json.contains(kJsonUsernameProp) && json[kJsonUsernameProp].isString()) {
        username_ = json[kJsonUsernameProp].toString();
    }

    if (json.contains(kJsonPasswordProp) && json[kJsonPasswordProp].isString()) {
        password_ = Utils::fromBase64(json[kJsonPasswordProp].toString());
    }
}

void ProxySettings::fromIni(const QSettings &settings)
{
    option_ = PROXY_OPTION_fromString(settings.value(kIniOptionProp, PROXY_OPTION_toString(option_)).toString());
    address_ = settings.value(kIniAddressProp).toString();
    port_ = settings.value(kIniPortProp, 0).toUInt();
    username_ = settings.value(kIniUsernameProp).toString();
    password_ = settings.value(kIniPasswordProp).toString();
}

void ProxySettings::toIni(QSettings &settings) const
{
    settings.setValue(kIniOptionProp, PROXY_OPTION_toString(option_));
    settings.setValue(kIniAddressProp, address_);
    settings.setValue(kIniPortProp, static_cast<int>(port_));
    settings.setValue(kIniUsernameProp, username_);
    settings.setValue(kIniPasswordProp, password_);
}

QJsonObject ProxySettings::toJson(bool isForDebugLog) const
{
    QJsonObject json;
    json[kJsonOptionProp] = static_cast<int>(option_);
    json[kJsonAddressProp] = address_;
    json[kJsonPortProp] = static_cast<int>(port_);
    if (isForDebugLog) {
        json["optionDesc"] = PROXY_OPTION_toString(option_);
    } else {
        json[kJsonUsernameProp] = username_;
        json[kJsonPasswordProp] = Utils::toBase64(password_);
    }
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

QString ProxySettings::curlAddress() const
{
    if (option_ == PROXY_OPTION_HTTP)
        return "http://" + address_ + ":" + QString::number(port_);
    else if (option_ == PROXY_OPTION_SOCKS)
        return "socks5://" + address_ + ":" + QString::number(port_);
    else
        return "";
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

void ProxySettings::validate()
{
    option_ = PROXY_OPTION_fromInt(static_cast<int>(option_));
    if (option_ == PROXY_OPTION_AUTODETECT || option_ == PROXY_OPTION_SOCKS) {
        qCWarning(LOG_BASIC) << "ProxySettings: unsupported proxy option, resetting";
        option_ = PROXY_OPTION_NONE;
    }

    bool needsReset = false;
    if (option_ == PROXY_OPTION_HTTP) {
        if (!NetworkingValidation::isIp(address_) && !NetworkingValidation::isDomain(address_)) {
            qCWarning(LOG_BASIC) << "ProxySettings: invalid address, disabling proxy";
            needsReset = true;
        } else if (!NetworkingValidation::isValidPort(static_cast<int>(port_))) {
            qCWarning(LOG_BASIC) << "ProxySettings: invalid port, disabling proxy";
            needsReset = true;
        }
    }

    constexpr int kMaxCredentialLen = 255;
    if (username_.size() > kMaxCredentialLen || username_.contains(QChar(0)) ||
        password_.size() > kMaxCredentialLen || password_.contains(QChar(0))) {
        qCWarning(LOG_BASIC) << "ProxySettings: credentials out of bounds, disabling proxy";
        needsReset = true;
    }

    if (needsReset) {
        option_ = PROXY_OPTION_NONE;
        address_.clear();
        port_ = 0;
        username_.clear();
        password_.clear();
    }
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

} //namespace types
