#include "bridgeapi_utils.h"

#include <assert.h>

#include "bridgeapi_request.h"

namespace wsnet {

std::shared_ptr<WSNetHttpRequest> serverapi_utils::createHttpRequest(WSNetHttpNetworkManager *httpNetworkManager, const std::string &domain, BaseRequest *request, bool bIgnoreSslErrors, bool isAPIExtraTLSPadding)
{
    // Make sure the network return code is reset
    request->setRetCode(ApiRetCode::kSuccess);

    std::shared_ptr<WSNetHttpRequest> httpRequest;
    switch (request->requestType()) {
    case HttpMethod::kGet:
        httpRequest = httpNetworkManager->createGetRequest(request->url(domain), request->timeout(), bIgnoreSslErrors);
        break;
    case HttpMethod::kPost:
        httpRequest = httpNetworkManager->createPostRequest(request->url(domain), request->timeout(), request->postData(), bIgnoreSslErrors);
        break;
    case HttpMethod::kDelete:
        httpRequest = httpNetworkManager->createDeleteRequest(request->url(domain), request->timeout(), bIgnoreSslErrors);
        break;
    case HttpMethod::kPut:
        httpRequest = httpNetworkManager->createPutRequest(request->url(domain), request->timeout(), request->postData(), bIgnoreSslErrors);
        break;
    default:
        assert(false);
    }

    if (!request->isUseDnsCache()) {
        httpRequest->setUseDnsCache(false);
    }

    if (isAPIExtraTLSPadding) {
        httpRequest->setExtraTLSPadding(true);
    }

    const std::string token = static_cast<BridgeAPIRequest *>(request)->sessionToken();
    if (!token.empty()) {
        httpRequest->setSessionToken(token);
    }

    return httpRequest;
}

} // namespace wsnet
