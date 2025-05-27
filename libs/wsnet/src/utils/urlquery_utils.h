#pragma once

#include <skyr/url.hpp>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace wsnet {

namespace urlquery_utils
{
    void addPlatformQueryItems(skyr::url_search_parameters &sp);
    void addAuthQueryItems(skyr::url_search_parameters &sp);
    std::unordered_map<std::string, std::string> buildCaptchaParams(const std::string& secureToken, const std::string& captchaSolution,
                                                                    const std::vector<float>& captchaTrailX, const std::vector<float>& captchaTrailY);
}

} // namespace wsnet
