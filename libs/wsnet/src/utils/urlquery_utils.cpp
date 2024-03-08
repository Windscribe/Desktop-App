#include "urlquery_utils.h"
#include "crypto_utils.h"
#include "settings.h"

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

} // namespace wsnet
