#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "scapix_object.h"

namespace wsnet {

class WSNetDnsRequestResult : public scapix_object<WSNetDnsRequestResult>
{
public:
    virtual ~WSNetDnsRequestResult() {}

    virtual std::vector<std::string> ips() = 0;
    virtual std::uint32_t elapsedMs() = 0;
    virtual bool isError() = 0;
    virtual std::string errorString() = 0;
};

} // namespace wsnet
