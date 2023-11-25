#pragma once

#include <QObject>
#include <QVector>
#include "types/portmap.h"
#include "types/enums.h"

class PreferencesHelper : public QObject
{
    Q_OBJECT
public:
    explicit PreferencesHelper(QObject *parent = nullptr);

    QString buildVersion();
    QList<QPair<QString, QVariant>> availableLanguages() const;

    void setProxyGatewayAddress(const QString &address);
    QString getProxyGatewayAddress() const;

    QVector<TAP_ADAPTER_TYPE> getAvailableTapAdapters(const QString &openVpnVersion);

    void setPortMap(const types::PortMap &portMap);
    QVector<types::Protocol> getAvailableProtocols();
    QVector<uint> getAvailablePortsForProtocol(types::Protocol protocol);

    void setWifiSharingSupported(bool bSupported);
    bool isWifiSharingSupported() const;

    void setIsDockedToTray(bool b);
    bool isDockedToTray() const;

    void setIsExternalConfigMode(bool b);
    bool isExternalConfigMode() const;

#ifdef Q_OS_WIN
    void setIpv6StateInOS(bool bEnabled);
    bool getIpv6StateInOS() const;
#endif

signals:
    void portMapChanged();
    void wifiSharingSupportedChanged(bool bSupported);
    void proxyGatewayAddressChanged(const QString &address);
    void ipv6StateInOSChanged(bool bEnabled);
    void installedTapAdapterChanged(TAP_ADAPTER_TYPE tapAdapter);
    void isDockedModeChanged(bool bIsDockedToTray);
    void isExternalConfigModeChanged(bool bIsExternalConfigMode);

private:
    QStringList availableLanguageCodes_;

    types::PortMap portMap_;
    bool isWifiSharingSupported_;

    QString proxyGatewayAddress_;
    bool bIpv6StateInOS_;

    bool isDockedToTray_;
    bool isExternalConfigMode_;
};
