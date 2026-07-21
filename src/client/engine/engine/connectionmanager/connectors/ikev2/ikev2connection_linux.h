#pragma once

#include <QObject>

#include "engine/connectionmanager/connectors/ikev2/ikev2connectionbase.h"
#include "engine/helper/helper.h"

class IKEv2Connection_linux : public Ikev2ConnectionBase
{
    Q_OBJECT
public:
    explicit IKEv2Connection_linux(QObject *parent, Helper *helper, types::Protocol protocol, const Ikev2SessionParams &sessionParams);
    ~IKEv2Connection_linux() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    //QString getConnectedTapTunAdapterName() override;

private slots:
    void fakeImpl();

};
