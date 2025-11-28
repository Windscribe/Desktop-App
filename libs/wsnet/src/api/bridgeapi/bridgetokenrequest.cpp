#include "bridgetokenrequest.h"
#include <nlohmann/json.hpp>
#include "utils/wsnet_logger.h"

namespace wsnet {

BridgeTokenRequest::BridgeTokenRequest(const std::string &sessionToken, RequestFinishedCallback callback) :
    BridgeAPIRequest(HttpMethod::kPost, SubdomainType::kBridgeAPI, "v1/token", std::map<std::string, std::string>(), sessionToken, callback)
{
    setTimeout(5000);
}

void BridgeTokenRequest::handle(const std::string &arr)
{
    // Call parent handle first
    BridgeAPIRequest::handle(arr);

    // If parent handle failed, don't proceed
    if (retCode() != ApiRetCode::kSuccess) {
        return;
    }

    // Parse the token from the response
    try {
        nlohmann::json j = nlohmann::json::parse(arr);
        if (j.contains("session_token")) {
            sessionTokenResponse_ = j["session_token"];
            g_logger->info("BridgeTokenRequest: token parsed successfully");
        } else if (j.contains("token")) {
            sessionTokenResponse_ = j["token"];
            g_logger->info("BridgeTokenRequest: token parsed successfully");
        } else {
            g_logger->error("BridgeTokenRequest: invalid token response format");
            setRetCode(ApiRetCode::kIncorrectJson);
        }
    } catch (const std::exception &e) {
        g_logger->error("BridgeTokenRequest: token JSON parse error: {}", e.what());
        setRetCode(ApiRetCode::kIncorrectJson);
    }
}

const std::string& BridgeTokenRequest::getSessionToken() const
{
    return sessionTokenResponse_;
}

} // namespace wsnet