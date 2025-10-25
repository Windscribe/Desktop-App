#pragma once
#include "../baserequest.h"
#include "bridgeapi_request.h"

namespace wsnet {

namespace bridgeapi_requests_factory
{
    BaseRequest *pinIp(const std::string &sessionToken, const std::string &ip, RequestFinishedCallback callback);
    BaseRequest *rotateIp(const std::string &sessionToken, RequestFinishedCallback callback);
    BaseRequest *fetchToken(const std::string &sessionToken, RequestFinishedCallback callback);
}

} // namespace wsnet
