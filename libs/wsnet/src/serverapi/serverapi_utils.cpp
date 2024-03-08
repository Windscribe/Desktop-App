#include "serverapi_utils.h"

namespace wsnet {

std::shared_ptr<WSNetHttpRequest> serverapi_utils::createHttpRequestWithFailoverParameters(WSNetHttpNetworkManager *httpNetworkManager, const FailoverData &failoverData, BaseRequest *request,
                                                                                           bool bIgnoreSslErrors, bool isAPIExtraTLSPadding)
{
    // Make sure the network return code is reset
    request->setRetCode(ServerApiRetCode::kSuccess);

    std::shared_ptr<WSNetHttpRequest> httpRequest;
    switch (request->requestType()) {
    case HttpMethod::kGet:
        httpRequest = httpNetworkManager->createGetRequest(request->url(failoverData.domain()), request->timeout(), bIgnoreSslErrors);
        break;
    case HttpMethod::kPost:
        httpRequest = httpNetworkManager->createPostRequest(request->url(failoverData.domain()), request->timeout(), request->postData(), bIgnoreSslErrors);
        break;
    case HttpMethod::kDelete:
        httpRequest = httpNetworkManager->createDeleteRequest(request->url(failoverData.domain()), request->timeout(), bIgnoreSslErrors);
        break;
    case HttpMethod::kPut:
        httpRequest = httpNetworkManager->createPutRequest(request->url(failoverData.domain()), request->timeout(), request->postData(), bIgnoreSslErrors);
        break;
    default:
        assert(false);
    }

    if (!failoverData.echConfig().empty())
        httpRequest->setEchConfig(failoverData.echConfig());

    if (isAPIExtraTLSPadding)
        httpRequest->setExtraTLSPadding(true);

    if (!failoverData.sniDomain().empty())
        httpRequest->setSniDomain(failoverData.sniDomain());

    return httpRequest;
}


} // namespace wsnet
