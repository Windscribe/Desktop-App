#ifndef OPENVPNCONNECTION_LINUX_H
#define OPENVPNCONNECTION_LINUX_H

#include <QThread>
#include <QElapsedTimer>
#include <QTimer>
#include <QMutex>
#include "engine/helper/ihelper.h"
#include "iconnection.h"
#include "engine/proxy/proxysettings.h"
#include "utils/boost_includes.h"
#include <atomic>

class OpenVPNConnection_linux : public IConnection
{
    Q_OBJECT

public:
    explicit OpenVPNConnection_linux(QObject *parent, IHelper *helper);
    ~OpenVPNConnection_linux() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    bool isAllowFirewallAfterCustomConfigConnection() const override;

    ConnectionType getConnectionType() const override { return ConnectionType::OPENVPN; }

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;

protected:
    void run() override;

private slots:
    void onTimer();

private:
    bool isConnected_;
    QTimer timer_;
};

#endif // OPENVPNCONNECTION_LINUX_H
