#pragma once

#include <string>
#include <vector>
#include <functional>
#include "scapix_object.h"
#include "WSNetCancelableCallback.h"
#include "WSNetServerAPI.h"

namespace wsnet {

// Useful for testing and debugging purposes
class WSNetUtils : public scapix_object<WSNetUtils>
{
public:
    virtual ~WSNetUtils() {}

    virtual std::int32_t failoverCount() const = 0;
    virtual std::string failoverName(int failoverInd) const = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> myIPViaFailover(int failoverInd, WSNetRequestFinishedCallback callback) = 0;
};

} // namespace wsnet
