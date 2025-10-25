#pragma once
#include <string>
#include <mutex>

namespace wsnet {

// Stores persistent settings for lib. Uses the json format.
// thread safe
class PersistentSettings
{
public:
    explicit PersistentSettings(const std::string &settings);

    // empty string means no value for all functions
    void setFailoverId(const std::string &failoverId);
    std::string failoverId() const;

    void setCountryOverride(const std::string &countryOverride);
    std::string countryOverride() const;

    void setAuthHash(const std::string &authHash);
    std::string authHash() const;

    void setSessionStatus(const std::string &sessionStatus);
    std::string sessionStatus() const;

    void setLocations(const std::string &locations);
    std::string locations() const;

    void setServerCredentialsOvpn(const std::string &serverCredentials);
    std::string serverCredentialsOvpn() const;

    void setServerCredentialsIkev2(const std::string &serverCredentials);
    std::string serverCredentialsIkev2() const;

    // openvpn config
    void setServerConfigs(const std::string &serverConfigs);
    std::string serverConfigs() const;

    void setPortMap(const std::string &portMap);
    std::string portMap() const;

    void setStaticIps(const std::string &staticIps);
    std::string staticIps() const;

    void setNotifications(const std::string &notifications);
    std::string notifications() const;

    void setSessionToken(const std::string &sessionToken);
    std::string sessionToken() const;

    std::string getAsString() const;

private:
    // should increment the version if the data format is changed
    static constexpr int kVersion = 1;

    std::string failoverId_;
    std::string countryOverride_;

    std::string authHash_;
    std::string sessionStatus_;
    std::string locations_;
    std::string serverCredentialsOvpn_;
    std::string serverCredentialsIkev2_;
    std::string serverConfigs_;
    std::string portMap_;
    std::string staticIps_;
    std::string notifications_;
    std::string sessionToken_;

    mutable std::mutex mutex_;
};

} // namespace wsnet
