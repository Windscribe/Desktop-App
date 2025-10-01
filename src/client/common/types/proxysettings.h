#pragma once

#include <QJsonObject>
#include <QNetworkProxy>
#include <QSettings>
#include <QString>
#include "enums.h"

namespace types {

class ProxySettings
{
public:
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

    void fromIni(const QSettings &settings);
    void toIni(QSettings &settings) const;
    QJsonObject toJson(bool isForDebugLog) const;

    friend QDataStream& operator <<(QDataStream &stream, const ProxySettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ProxySettings &o);

private:
    PROXY_OPTION option_ = PROXY_OPTION_NONE;
    QString address_;
    uint port_ = 0;
    QString username_;
    QString password_;

    static const inline QString kIniOptionProp = "ProxyMode";
    static const inline QString kIniAddressProp = "ProxyAddress";
    static const inline QString kIniPortProp = "ProxyPort";
    static const inline QString kIniUsernameProp = "ProxyUsername";
    static const inline QString kIniPasswordProp = "ProxyPassword";

    static const inline QString kJsonOptionProp = "option";
    static const inline QString kJsonAddressProp = "address";
    static const inline QString kJsonPortProp = "port";
    static const inline QString kJsonUsernameProp = "username";
    static const inline QString kJsonPasswordProp = "password";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types
