#pragma once

#include <string>
#include "scapix_object.h"

namespace wsnet {

enum class Protocol {
    kUdp = 0,
    kTcp
};

class WSNetEmergencyConnectEndpoint : public scapix_object<WSNetEmergencyConnectEndpoint>
{
public:
    virtual ~WSNetEmergencyConnectEndpoint() {}

    virtual std::string ip() const = 0;
    virtual std::uint16_t port() const = 0;
    virtual Protocol protocol() const = 0;
};

} // namespace wsnet
