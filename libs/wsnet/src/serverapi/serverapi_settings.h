#pragma once
#include <string>
#include <mutex>

namespace wsnet {

// Stores persistent settings for ServerAPI. Uses the json format.
// thread safe
class ServerAPISettings
{
public:
    explicit ServerAPISettings(const std::string &settings);

    // empty string means no value
    void setFailovedId(const std::string &failoverId);
    std::string failoverId() const;

    // empty string means no value
    void setCountryOverride(const std::string &countryOverride);
    std::string countryOverride() const;

    std::string getAsString() const;

private:
    // should increment the version if the data format is changed
    static constexpr int kVersion = 1;

    std::string failoverId_;
    std::string countryOverride_;
    mutable std::mutex mutex_;
};

} // namespace wsnet
