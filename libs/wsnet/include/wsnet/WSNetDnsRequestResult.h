#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "scapix_object.h"

#include "WSNetRequestError.h"

namespace wsnet {

class WSNetDnsRequestResult : public scapix_object<WSNetDnsRequestResult>
{
public:
    virtual ~WSNetDnsRequestResult() {}

    virtual std::vector<std::string> ips() const = 0;
    virtual std::uint32_t elapsedMs() const = 0;
    virtual std::shared_ptr<WSNetRequestError> error() const  = 0;
};

} // namespace wsnet
