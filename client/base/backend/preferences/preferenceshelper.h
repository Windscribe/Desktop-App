#pragma once

#include <QObject>
#include <QVector>
#include "api_responses/portmap.h"
#include "types/enums.h"
#include "types/protocol.h"

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

    void setPortMap(const api_responses::PortMap &portMap);
    QVector<types::Protocol> getAvailableProtocols();
    QVector<uint> getAvailablePortsForProtocol(types::Protocol protocol);

    void setWifiSharingSupported(bool bSupported);
    bool isWifiSharingSupported() const;

    void setIsDockedToTray(bool b);
    bool isDockedToTray() const;

    void setIsExternalConfigMode(bool b);
    bool isExternalConfigMode() const;

    void setCurrentProtocol(const types::Protocol &protocol);
    types::Protocol currentProtocol() const;

signals:
    void portMapChanged();
    void wifiSharingSupportedChanged(bool bSupported);
    void proxyGatewayAddressChanged(const QString &address);
    void installedTapAdapterChanged(TAP_ADAPTER_TYPE tapAdapter);
    void isDockedModeChanged(bool bIsDockedToTray);
    void isExternalConfigModeChanged(bool bIsExternalConfigMode);
    void currentProtocolChanged(const types::Protocol &protocol);

private:
    QStringList availableLanguageCodes_;

    api_responses::PortMap portMap_;
    bool isWifiSharingSupported_;

    QString proxyGatewayAddress_;

    bool isDockedToTray_;
    bool isExternalConfigMode_;

    types::Protocol currentProtocol_;
};
