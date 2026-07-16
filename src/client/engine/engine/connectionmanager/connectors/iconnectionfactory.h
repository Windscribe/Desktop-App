#pragma once

#include <QObject>

#include "engine/helper/helper.h"
#include "types/protocol.h"

class IConnection;

// Creates the protocol-specific connector. Seam so ConnectionManager stays free of platform
// #ifdefs and tests can substitute fake connectors.
class IConnectionFactory
{
public:
    virtual ~IConnectionFactory() {}

    virtual IConnection *createConnection(types::Protocol protocol, QObject *parent, Helper *helper) = 0;
    virtual void removeIkev2ConnectionFromOS() = 0;
};
