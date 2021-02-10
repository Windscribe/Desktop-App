#ifndef PREFERENCESHELPER_H
#define PREFERENCESHELPER_H

#include <QObject>
#include <QVector>
#include "utils/protobuf_includes.h"

class PreferencesHelper : public QObject
{
    Q_OBJECT
public:
    explicit PreferencesHelper(QObject *parent = nullptr);

    QString buildVersion();
    QList<QPair<QString, QString> > availableLanguages();

    void setProxyGatewayAddress(const QString &address);
    QString getProxyGatewayAddress() const;

    void setAvailableOpenVpnVersions(const QStringList &list);
    QStringList getAvailableOpenVpnVersions();

    QVector<ProtoTypes::TapAdapterType> getAvailableTapAdapters(const QString &openVpnVersion);

    void setPortMap(const ProtoTypes::ArrayPortMap &arr);
    QVector<ProtoTypes::Protocol> getAvailableProtocols();
    QVector<uint> getAvailablePortsForProtocol(ProtoTypes::Protocol protocol);

    void setWifiSharingSupported(bool bSupported);
    bool isWifiSharingSupported() const;

    void setBlockFirewall(bool b);
    bool isFirewallBlocked() const;

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
    void availableOpenVpnVersionsChanged(const QStringList &list);
    void wifiSharingSupportedChanged(bool bSupported);
    void proxyGatewayAddressChanged(const QString &address);
    void ipv6StateInOSChanged(bool bEnabled);
    void installedTapAdapterChanged(ProtoTypes::TapAdapterType tapAdapter);
    void isFirewallBlockedChanged(bool bFirewallBlocked);
    void isDockedModeChanged(bool bIsDockedToTray);
    void isExternalConfigModeChanged(bool bIsExternalConfigMode);

private:
    QStringList availableLanguageCodes_;
    QStringList availableOpenVpnVersions_;

    ProtoTypes::ArrayPortMap portMap_;
    bool isWifiSharingSupported_;

    QString proxyGatewayAddress_;
    bool bIpv6StateInOS_;

    bool isFirewallBlocked_;
    bool isDockedToTray_;
    bool isExternalConfigMode_;
};

#endif // PREFERENCESHELPER_H
