#ifndef IKEV2CONNECTION_TEST_H
#define IKEV2CONNECTION_TEST_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QStateMachine>
#include "engine/types/types.h"
#include "iconnection.h"
#include "engine/helper/ihelper.h"

// mock ikev2 connection object for testing
class IKEv2Connection_test : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_test(QObject *parent, IHelper *helper);
    ~IKEv2Connection_test() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,  const QString &username, const QString &password, const ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    ConnectionType getConnectionType() const override { return ConnectionType::IKEV2; }

    static void removeIkev2ConnectionFromOS();

private:
    bool isConnected_;
    QStateMachine *stateMachine_;
};

#endif // IKEV2CONNECTION_TEST_H
