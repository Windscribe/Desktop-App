#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/helper/helper.h"

class IKEv2Connection_linux : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_linux(QObject *parent, Helper *helper);
    ~IKEv2Connection_linux() override;

    void startConnect(const StartConnectParams &params) override;
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
