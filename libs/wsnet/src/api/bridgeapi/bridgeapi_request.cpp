#include "bridgeapi_request.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <skyr/url.hpp>
#include "utils/urlquery_utils.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

BridgeAPIRequest::BridgeAPIRequest(HttpMethod requestType, SubdomainType subDomainType, const std::string &name,
        std::map<std::string, std::string> extraParams, const std::string &sessionToken, RequestFinishedCallback callback) :
    BaseRequest(requestType, subDomainType, RequestPriority::kNormal, name, extraParams, callback),
    sessionToken_(sessionToken)
{
    setTimeout(kRequestTimeoutMs);
}

void BridgeAPIRequest::setSessionToken(const std::string &token)
{
    sessionToken_ = token;
}

std::string BridgeAPIRequest::sessionToken() const
{
    return sessionToken_;
}

std::string BridgeAPIRequest::url(const std::string &domain) const
{
    auto url = skyr::url("https://" + hostname(domain, subDomainType_) + "/" + name());
    if (requestType_ == HttpMethod::kGet || requestType_ == HttpMethod::kDelete) {
        auto &sp = url.search_parameters();
        for (auto &it : extraParams_) {
            if (!it.second.empty()) {
                sp.set(it.first, it.second);
            }
        }
    }
    return url.c_str();
}

std::string BridgeAPIRequest::postData() const
{
    if (requestType_ == HttpMethod::kPost || requestType_ == HttpMethod::kPut) {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        for (const auto& param : extraParams_) {
            if (!param.second.empty()) {
                rapidjson::Value key(param.first.c_str(), allocator);
                rapidjson::Value value(param.second.c_str(), allocator);
                doc.AddMember(key, value, allocator);
            }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();
    }
    return std::string();
}

std::string BridgeAPIRequest::hostname(const std::string &domain, SubdomainType subdomain) const
{
    return domain;
}

void BridgeAPIRequest::handle(const std::string &arr)
{
    json_ = arr;

    if (arr.empty()) {
        setRetCode(ApiRetCode::kIncorrectJson);
        g_logger->info("Received an empty json response, return ApiRetCode::kIncorrectJson");
        return;
    }

    if (!isIgnoreJsonParse_) {
        using namespace rapidjson;
        Document doc;
        doc.Parse(arr.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            g_logger->info("Received an incorrect json response: {}", arr);
            setRetCode(ApiRetCode::kIncorrectJson);
            return;
        }
    }
}

} // namespace wsnet
