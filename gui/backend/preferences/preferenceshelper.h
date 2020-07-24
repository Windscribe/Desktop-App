#ifndef PREFERENCESHELPER_H
#define PREFERENCESHELPER_H

#include <QObject>
#include <QVector>
#include "ipc/generated_proto/types.pb.h"

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

private:
    QStringList availableLanguageCodes_;
    QStringList availableOpenVpnVersions_;

    ProtoTypes::ArrayPortMap portMap_;
    bool isWifiSharingSupported_;

    QString proxyGatewayAddress_;
    bool bIpv6StateInOS_;

    bool isFirewallBlocked_;
};

#endif // PREFERENCESHELPER_H
