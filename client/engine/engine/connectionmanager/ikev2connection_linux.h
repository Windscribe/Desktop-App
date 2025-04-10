#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include "engine/helper/helper.h"
#include "iconnection.h"

class IKEv2Connection_linux : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_linux(QObject *parent, Helper *helper);
    ~IKEv2Connection_linux() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,  const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode, bool isCustomConfig, const QString &overrideDnsIp) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    //QString getConnectedTapTunAdapterName() override;
    ConnectionType getConnectionType() const override { return ConnectionType::IKEV2; }

    static void removeIkev2ConnectionFromOS();

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;

private slots:
    void fakeImpl();

};
