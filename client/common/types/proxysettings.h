#pragma once

#include <QSettings>
#include <QString>
#include <QNetworkProxy>
#include <QJsonObject>
#include "enums.h"

namespace types {

class ProxySettings
{
public:

    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kOptionProp = "option";
        const QString kAddressProp = "address";
        const QString kPortProp = "port";
        const QString kUsernameProp = "username";
        const QString kPasswordProp = "password";
    };

    ProxySettings();
    ProxySettings(PROXY_OPTION option, const QString &address, uint port, const QString &password, const QString &username);
    ProxySettings(const QJsonObject &json);

    PROXY_OPTION option() const;
    void setOption(PROXY_OPTION option);

    // address example: https://1.2.3.4:8083, details: https://curl.se/libcurl/c/CURLOPT_PROXY.html
    QString curlAddress() const;

    QString address() const;
    void setAddress(const QString &address);

    uint getPort() const;
    void setPort(const uint &value);

    QString getUsername() const;
    void setUsername(const QString &username);

    QString getPassword() const;
    void setPassword(const QString &password);

    QNetworkProxy getNetworkProxy() const;
    bool isProxyEnabled() const;

    bool operator==(const ProxySettings &other) const {
        return option_ == other.option_ && address_ == other.address_ && port_ == other.port_
               && username_ == other.username_ && password_ == other.password_;
    }

    bool operator!=(const ProxySettings &other) const {
        return !(*this == other);
    }

    QJsonObject toJson() const;

    friend QDataStream& operator <<(QDataStream &stream, const ProxySettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ProxySettings &o);

    friend QDebug operator<<(QDebug dbg, const ProxySettings &ps);

private:
    PROXY_OPTION option_;
    QString address_;
    uint port_;
    QString username_;
    QString password_;
    JsonInfo jsonInfo_;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types
