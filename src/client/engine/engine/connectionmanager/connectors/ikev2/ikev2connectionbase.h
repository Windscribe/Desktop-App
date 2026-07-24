#pragma once

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/connectionmanager/connectors/ikev2/ikev2sessionparams.h"

// Platform-independent IKEv2 preparation: the ExtraConfig endpoint override and the credential pick.
// Platform classes derive from this and dial with descr_/username()/password().
class Ikev2ConnectionBase : public IConnection
{
    Q_OBJECT

public:
    Ikev2ConnectionBase(QObject *parent, types::Protocol protocol, const Ikev2SessionParams &sessionParams);

protected:
    void prepareImpl() override;

    QString username() const { return username_; }
    QString password() const { return password_; }

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    friend class TestIkev2Connection;
#endif

    Ikev2SessionParams sessionParams_;
    QString username_;
    QString password_;
};
