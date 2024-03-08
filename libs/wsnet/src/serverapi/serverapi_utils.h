#pragma once

#include "WSNetHttpNetworkManager.h"
#include "failover/failoverdata.h"
#include "baserequest.h"

namespace wsnet {

namespace serverapi_utils {
    std::shared_ptr<WSNetHttpRequest> createHttpRequestWithFailoverParameters(WSNetHttpNetworkManager *httpNetworkManager, const FailoverData &failoverData, BaseRequest *request,
                                                                          bool bIgnoreSslErrors, bool isAPIExtraTLSPadding);
}

} // namespace wsnet
