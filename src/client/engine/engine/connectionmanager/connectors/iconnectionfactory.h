#pragma once

#include <QObject>

#include "engine/helper/helper.h"
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

    virtual IConnection *createConnection(types::Protocol protocol, QObject *parent, Helper *helper, const ConnectRequest &request) = 0;
    virtual void removeIkev2ConnectionFromOS() = 0;
};
