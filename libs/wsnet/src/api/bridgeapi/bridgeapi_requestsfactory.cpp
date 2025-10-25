#include "bridgeapi_requestsfactory.h"
#include "bridgeapi_request.h"
#include "bridgetokenrequest.h"
#include "utils/utils.h"

namespace wsnet {

BaseRequest *bridgeapi_requests_factory::pinIp(const std::string &sessionToken, const std::string &ip, RequestFinishedCallback callback)
{
    std::map<std::string, std::string> extraParams;
    extraParams["ip"] = ip;

    auto request = new BridgeAPIRequest(HttpMethod::kPost, SubdomainType::kBridgeAPI, "v1/ip/pin", extraParams, sessionToken, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *bridgeapi_requests_factory::rotateIp(const std::string &sessionToken, RequestFinishedCallback callback)
{
    auto request = new BridgeAPIRequest(HttpMethod::kPost, SubdomainType::kBridgeAPI, "v1/ip/rotate", std::map<std::string, std::string>(), sessionToken, callback);
    request->setContentTypeHeader("Content-type: text/html; charset=utf-8");
    return request;
}

BaseRequest *bridgeapi_requests_factory::fetchToken(const std::string &sessionToken, RequestFinishedCallback callback)
{
    return new BridgeTokenRequest(sessionToken, callback);
}

} // namespace wsnet
