#pragma once
#include <string>

// ADVobfuscator include file
#include <Lib/MetaString.h>

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

    void setBasePlatform(const std::string &platform)
    {
        basePlatform_ = platform;
    }

    std::string basePlatform() const { return basePlatform_; }


    void setDeviceId(const std::string &deviceId)
    {
        deviceId_ = deviceId;
    }

    std::string deviceId() const { return deviceId_; }


    void setAppVersion(const std::string &appVersion)
    {
        appVersion_ = appVersion;
    }

    std::string appVersion() const { return appVersion_; }

    void setLanguage(const std::string &language)
    {
        language_ = language;
    }

    std::string language() const { return language_; }

    void setOpenVpnVersion(const std::string &openVpnVersion)
    {
        openVpnVersion_ = openVpnVersion;
    }

    std::string openVpnVersion() const { return openVpnVersion_; }


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

    std::string serverSharedKey() const { return OBFUSCATED("952b4412f002315aa50751032fcaab03"); }

private:
    Settings() {};
    bool isUseStaging_ = false;
    std::string platformName_;
    std::string basePlatform_;
    std::string deviceId_;
    std::string appVersion_;
    std::string openVpnVersion_;
    std::string language_;
};

} // namespace wsnet
