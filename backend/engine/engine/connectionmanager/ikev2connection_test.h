#ifndef IKEV2CONNECTION_TEST_H
#define IKEV2CONNECTION_TEST_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QStateMachine>
#include "Engine/Types/types.h"
#include "IConnection.h"
#include "Engine/Helper/ihelper.h"

// mock ikev2 connection object for testing
class IKEv2Connection_test : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_test(QObject *parent, IHelper *helper);
    virtual ~IKEv2Connection_test();

    virtual void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,  const QString &username, const QString &password, const ProxySettings &proxySettings,
                              bool isEnableIkev2Compression, bool isAutomaticConnectionMode);
    virtual void startDisconnect();
    virtual bool isDisconnected();

    virtual QString getConnectedTapAdapterName();
    static void removeIkev2ConnectionFromOS();

private:
    bool isConnected_;
    QStateMachine *stateMachine_;
};

#endif // IKEV2CONNECTION_TEST_H
