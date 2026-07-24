#pragma once

#include "engine/connectionmanager/connectors/iconnectionfactory.h"
#include "engine/helper/helper.h"

class ConnectionFactory : public IConnectionFactory
{
public:
    explicit ConnectionFactory(Helper *helper) : helper_(helper) {}

    IConnection *createConnection(types::Protocol protocol, QObject *parent, const ConnectRequest &request) override;
    void removeIkev2ConnectionFromOS() override;
    bool hasUsableStoredConfig() const override;
    void removeStoredConfig() override;

    // Terminates VPN connections/processes left over from a previous app run (crashed or killed
    // instance). Static: it runs at engine startup, before any factory instance exists.
    static void finishActiveConnections(Helper *helper);

private:
    Helper *helper_;
};
