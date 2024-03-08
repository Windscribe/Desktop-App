#include "serverlocations_request.h"
#include <skyr/url.hpp>
#include <spdlog/spdlog.h>
#include <rapidjson/document.h>

namespace wsnet {

ServerLocationsRequest::ServerLocationsRequest(RequestPriority priority, const std::string &name,
        std::map<std::string, std::string> extraParams, ServerAPISettings &settings,
        ConnectState &connectState, WSNetAdvancedParameters *advancedParameters, RequestFinishedCallback callback) :
    BaseRequest(HttpMethod::kGet, SubdomainType::kAssets, priority, name, extraParams, callback),
    settings_(settings),
    connectState_(connectState),
    advancedParameters_(advancedParameters)
{
}

std::string ServerLocationsRequest::url(const std::string &domain) const
{
    isFromDisconnectedVPNState_ = !connectState_.isVPNConnected();

    auto url = skyr::url("https://" + hostname(domain, subDomainType_) + "/" + name());
    auto &sp = url.search_parameters();
    for (auto &it : extraParams_)
        if (!it.second.empty())
            sp.set(it.first, it.second);

    // country override logic
    std::string countryOverride;
    if (advancedParameters_->isIgnoreCountryOverride()) {
        // Instruct the serverlist endpoint to ignore geolocation based on our IP.
        countryOverride = "ZZ";
    }
    else {
        if (!advancedParameters_->countryOverrideValue().empty()) {
            countryOverride = advancedParameters_->countryOverrideValue();
        } else if (connectState_.isVPNConnected()) {
            if (!settings_.countryOverride().empty()) {
                countryOverride = settings_.countryOverride();
            }
        }
    }

    if (!countryOverride.empty()) {
        sp.set("country_override", countryOverride);
        spdlog::info("API request ServerLocations added countryOverride = {}", countryOverride);
    }

    return url.c_str();
}

void ServerLocationsRequest::handle(const std::string &arr)
{
    if (arr.empty()) {
        setRetCode(ServerApiRetCode::kIncorrectJson);
        return;
    }

    if (!isIgnoreJsonParse_) {
        using namespace rapidjson;
        Document doc;
        doc.Parse(arr.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            setRetCode(ServerApiRetCode::kIncorrectJson);
            return;
        }
        auto jsonObject = doc.GetObject();
        // all responses must contain errorCode or/and data fields
        if (!jsonObject.HasMember("errorCode") && !jsonObject.HasMember("data")) {
            setRetCode(ServerApiRetCode::kIncorrectJson);
            return;
        }

        // manage the country override flag according to the documentation
        // https://gitlab.int.windscribe.com/ws/client/desktop/client-desktop-public/-/issues/354
        if (!jsonObject.HasMember("info")) {
            setRetCode(ServerApiRetCode::kIncorrectJson);
            return;
        }

        auto jsonInfo = jsonObject["info"].GetObject();

        if (jsonInfo.HasMember("country_override")) {
            if (isFromDisconnectedVPNState_ && (!connectState_.isVPNConnected())) {
                auto countryOverride = jsonInfo["country_override"].GetString();
                settings_.setCountryOverride(countryOverride);
                spdlog::info("API request ServerLocations saved countryOverride = {}", countryOverride);
            }
        } else {
            if (isFromDisconnectedVPNState_ && (!connectState_.isVPNConnected())) {
                settings_.setCountryOverride(std::string());
                spdlog::info("API request ServerLocations removed countryOverride flag");
            }
        }
    }
    json_ = arr;
}



} // namespace wsnet

