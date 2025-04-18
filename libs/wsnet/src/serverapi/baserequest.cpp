#include "baserequest.h"
#include <assert.h>
#include <skyr/url.hpp>
#include <rapidjson/document.h>
#include "utils/wsnet_logger.h"
#include "settings.h"
#include "utils/utils.h"
#include "utils/urlquery_utils.h"

namespace wsnet {

BaseRequest::BaseRequest(HttpMethod requestType, SubdomainType subDomainType, RequestPriority priority, const std::string &name,
        std::map<std::string, std::string> extraParams, RequestFinishedCallback callback) :
    requestType_(requestType),
    subDomainType_(subDomainType),
    priority_(priority),
    name_(name),
    extraParams_(extraParams),
    callback_(callback)
{
}

std::string BaseRequest::url(const std::string &domain) const
{
    auto url = skyr::url("https://" + hostname(domain, subDomainType_) + "/" + name());
    if (requestType_ == HttpMethod::kGet || requestType_ == HttpMethod::kDelete) {
        auto &sp = url.search_parameters();
        for (auto &it : extraParams_)
            if (!it.second.empty())
                sp.set(it.first, it.second);

        urlquery_utils::addAuthQueryItems(sp);
        urlquery_utils::addPlatformQueryItems(sp);
    }
    return url.c_str();
}

std::string BaseRequest::postData() const
{
    if (requestType_ == HttpMethod::kPost || requestType_ == HttpMethod::kPut) {
        skyr::url_search_parameters sp;
        for (auto &it : extraParams_)
            if (!it.second.empty())
                sp.set(it.first, it.second);

        urlquery_utils::addAuthQueryItems(sp);
        urlquery_utils::addPlatformQueryItems(sp);
        return sp.to_string();
    }
    return std::string();
}

void BaseRequest::handle(const std::string &arr)
{
    json_ = arr;

    if (arr.empty()) {
        setRetCode(ServerApiRetCode::kIncorrectJson);
        g_logger->info("Received an empty json response, return ServerApiRetCode::kIncorrectJson");
        return;
    }

    if (!isIgnoreJsonParse_) {
        using namespace rapidjson;
        Document doc;
        doc.Parse(arr.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            g_logger->info("Received an incorrect json response: {}", arr);
            setRetCode(ServerApiRetCode::kIncorrectJson);
            return;
        }
        auto jsonObject = doc.GetObject();
        // all responses must contain errorCode or/and data fields
        if (!jsonObject.HasMember("errorCode") && !jsonObject.HasMember("data")) {
            g_logger->info("Received an incorrect json response: {}", arr);
            setRetCode(ServerApiRetCode::kIncorrectJson);
            return;
        }
    }
}

bool BaseRequest::isCanceled()
{
    return callback_->isCanceled();
}

void BaseRequest::callCallback()
{
    callback_->call(retCode_, json_);
}

std::string BaseRequest::hostname(const std::string &domain, SubdomainType subdomain) const
{
    // if this is IP, return without change
    if (utils::isIpAddress(domain)) {
        if (subdomain == SubdomainType::kAssets) {
            return domain + "/" + Settings::instance().serverAssetsSubdomain();
        } else if (subdomain == SubdomainType::kTunnelTest) {
            return domain + "/" + Settings::instance().serverTunnelTestSubdomain();
        } else {
            return domain;
        }
    }

    if (subdomain == SubdomainType::kApi)
        return Settings::instance().serverApiSubdomain() + "." + domain;
    else if (subdomain == SubdomainType::kAssets)
        return Settings::instance().serverAssetsSubdomain() + "." + domain;
    else if (subdomain == SubdomainType::kTunnelTest)
        return Settings::instance().serverTunnelTestSubdomain() + "." + domain;

    assert(false);
    return "";
}

} // namespace wsnet

