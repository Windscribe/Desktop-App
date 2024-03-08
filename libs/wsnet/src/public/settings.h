#pragma once
#include <string>

namespace wsnet {

class Settings
{
public:
    static Settings &instance()
    {
        static Settings s;
        return s;
    }

    void setUseStaging(bool isUseStaging)
    {
        isUseStaging_ = isUseStaging;
    }

    bool isStaging() const { return isUseStaging_; }

    void setPlatformName(const std::string &platformName)
    {
        platformName_ = platformName;
    }

    std::string platformName() const { return platformName_; }

    void setAppVersion(const std::string &appVersion)
    {
        appVersion_ = appVersion;
    }

    std::string appVersion() const { return appVersion_; }

    std::string primaryServerDomain() const
    {
        return "windscribe.com";
    }

    std::string serverApiSubdomain() const
    {
        if (isUseStaging_)
            return "api-staging";
        else
            return "api";
    }

    std::string serverAssetsSubdomain() const
    {
        if (isUseStaging_)
            return "assets-staging";
        else
            return "assets";
    }

    std::string serverTunnelTestSubdomain() const
    {
        return "checkip";
    }

    std::string serverUrl() const
    {
        if (isUseStaging_)
            return "www-staging.windscribe.com";
        else
            return "www.windscribe.com";
    }

    std::string serverSharedKey() const { return "952b4412f002315aa50751032fcaab03"; }

private:
    Settings() {};
    bool isUseStaging_ = false;
    std::string platformName_;
    std::string appVersion_;
};

} // namespace wsnet
