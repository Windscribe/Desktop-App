#pragma once

#include <QObject>

#include "types/protocol.h"

class IConnection;
struct ConnectRequest;

// Creates the protocol-specific connector. Seam so ConnectionManager stays free of platform
// #ifdefs and tests can substitute fake connectors. The factory is the sanctioned protocol-dispatch
// point: it hands each connector its per-protocol session params from the request.
class IConnectionFactory
{
public:
    virtual ~IConnectionFactory() {}

    virtual IConnection *createConnection(types::Protocol protocol, QObject *parent, const ConnectRequest &request) = 0;
    virtual void removeIkev2ConnectionFromOS() = 0;

    // Locally stored credentials for the config-fetching protocol family (today: the cached
    // WireGuard config used under Firewall Always On+). On the factory rather than IConnection
    // because the usability check runs before any connector exists — it decides whether the
    // attempt strategy may offer the protocol at all.
    virtual bool hasUsableStoredConfig() const = 0;
    virtual void removeStoredConfig() = 0;
};
