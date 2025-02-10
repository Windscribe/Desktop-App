#pragma once

#include "scapix_object.h"
#include <cstdint>


namespace wsnet {

// Providing decoy traffic functionality
class WSNetDecoyTraffic : public scapix_object<WSNetDecoyTraffic>
{
public:
    virtual ~WSNetDecoyTraffic() {}

    // 0 - low, 1 - medium, 2 - high
    virtual void setFakeTrafficVolume(std::uint32_t volume) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
};

} // namespace wsnet

