#ifndef PROXYSETTINGS_H
#define PROXYSETTINGS_H

#include <QSettings>
#include <QString>
#include <QNetworkProxy>
#include "engine/types/types.h"

class ProxySettings
{
public:
    ProxySettings();
    explicit ProxySettings(const ProtoTypes::ProxySettings &p);

    void readFromSettingsV1(QSettings &settings);

    bool isEqual(const ProxySettings &other) const;

    PROXY_OPTION option() const;
    void setOption(PROXY_OPTION option);

    QString address() const;
    void setAddress(const QString &address);

    uint getPort() const;
    void setPort(const uint &value);

    QString getUsername() const;
    void setUsername(const QString &username);

    QString getPassword() const;
    void setPassword(const QString &password);

    void debugToLog();

    QNetworkProxy getNetworkProxy() const;
    bool isProxyEnabled() const;

    ProtoTypes::ProxySettings convertToProtobuf() const;

    bool operator==(const ProxySettings &other) const {
        return option_ == other.option_ && address_ == other.address_ && port_ == other.port_
               && username_ == other.username_ && password_ == other.password_;
    }
    bool operator!=(const ProxySettings &other) const {
        return !(*this == other);
    }

private:
    PROXY_OPTION option_;
    QString address_;
    uint port_;
    QString username_;
    QString password_;
};

#endif // PROXYSETTINGS_H
