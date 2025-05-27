#include "urlquery_utils.h"
#include "crypto_utils.h"
#include "settings.h"
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace wsnet {

void urlquery_utils::addPlatformQueryItems(skyr::url_search_parameters &sp)
{
    sp.set("platform", Settings::instance().platformName());
    sp.set("app_version", Settings::instance().appVersion());
}

void urlquery_utils::addAuthQueryItems(skyr::url_search_parameters &sp)
{
    time_t timestamp;
    time(&timestamp);
    std::string strTimestamp = std::to_string(timestamp);
    std::string strHash = Settings::instance().serverSharedKey() + strTimestamp;
    std::string md5Hash = crypto_utils::md5(strHash);
    sp.set("time", strTimestamp);
    sp.set("client_auth_hash", md5Hash);
}

std::unordered_map<std::string, std::string> urlquery_utils::buildCaptchaParams(const std::string& secureToken, const std::string& captchaSolution,
                                                                    const std::vector<float>& captchaTrailX, const std::vector<float>& captchaTrailY)
{
      auto floatToString = [](float value) {
          std::ostringstream out;
          out << std::fixed << std::setprecision(3) << value;
          return out.str();
      };
      std::unordered_map<std::string, std::string> params;
      params["secure_token"] = secureToken;
      std::string signature = secureToken + Settings::instance().signatureToken();
      params["secure_token_sig"] = crypto_utils::sha256(signature);
      params["captcha_solution"] = captchaSolution;
      for (size_t i = 0; i < captchaTrailX.size(); ++i) {
          params["captcha_trail[x][" + std::to_string(i) + "]"] = floatToString(captchaTrailX[i]);
      }
      for (size_t i = 0; i < captchaTrailY.size(); ++i) {
          params["captcha_trail[y][" + std::to_string(i) + "]"] = floatToString(captchaTrailY[i]);
      }
      return params;
}

} // namespace wsnet
