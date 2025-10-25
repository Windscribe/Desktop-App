#pragma once

#include "../baserequest.h"
#include "WSNetHttpNetworkManager.h"

namespace wsnet {

namespace serverapi_utils {
    std::shared_ptr<WSNetHttpRequest> createHttpRequest(WSNetHttpNetworkManager *httpNetworkManager, const std::string &domain, BaseRequest *request, bool bIgnoreSslErrors, bool isAPIExtraTLSPadding);
}

} // namespace wsnet
