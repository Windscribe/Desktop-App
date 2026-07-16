#pragma once

#include "engine/connectionmanager/connectors/iconnectionfactory.h"

class ConnectionFactory : public IConnectionFactory
{
public:
    IConnection *createConnection(types::Protocol protocol, QObject *parent, Helper *helper) override;
    void removeIkev2ConnectionFromOS() override;
};
