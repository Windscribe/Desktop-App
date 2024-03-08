#pragma once

#include <map>
#include "WSNetEmergencyConnectEndpoint.h"

namespace wsnet {

class EmergencyConnectEndpoint : public WSNetEmergencyConnectEndpoint
{
public:
    explicit EmergencyConnectEndpoint(const std::string &ip, std::uint16_t port, Protocol protocol) :
        ip_(ip), port_(port), protocol_(protocol)
    {
    }

    std::string ip() const override
    {
        return ip_;
    }
    std::uint16_t port() const override
    {
        return port_;
    }
    Protocol protocol() const override
    {
        return protocol_;
    }

private:
    std::string ip_;
    std::uint16_t port_;
    Protocol protocol_;
};

} // namespace wsnet
