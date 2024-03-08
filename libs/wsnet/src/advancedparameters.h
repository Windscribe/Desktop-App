#pragma once
#include <mutex>

#include "WSNetAdvancedParameters.h"

namespace wsnet {

// Thread safe implementation of the WSNetAdvancedParameters interface
class AdvancedParameters : public WSNetAdvancedParameters
{
public:
    void setAPIExtraTLSPadding(bool isEnabled) override
    {
        std::lock_guard locker(mutex_);
        isAPIExtraTLSPadding_ = isEnabled;
    }
    bool isAPIExtraTLSPadding() const override
    {
        std::lock_guard locker(mutex_);
        return isAPIExtraTLSPadding_;
    }

    void setIgnoreCountryOverride(bool isIgnore) override
    {
        std::lock_guard locker(mutex_);
        isIgnoreCountryOverride_ = isIgnore;
    }

    bool isIgnoreCountryOverride() const override
    {
        std::lock_guard locker(mutex_);
        return isIgnoreCountryOverride_;
    }

    void setCountryOverrideValue(const std::string &country) override
    {
        std::lock_guard locker(mutex_);
        countryOverrideValue_ = country;
    }
    std::string countryOverrideValue() const override
    {
        std::lock_guard locker(mutex_);
        return countryOverrideValue_;
    }

    void setLogApiResponce(bool isEnabled) override
    {
        std::lock_guard locker(mutex_);
        isLogApiResponce_ = isEnabled;
    }
    bool isLogApiResponce() const override
    {
        std::lock_guard locker(mutex_);
        return isLogApiResponce_;
    }

private:
    mutable std::mutex mutex_;
    bool isAPIExtraTLSPadding_ = false;
    bool isIgnoreCountryOverride_ = false;
    std::string countryOverrideValue_;
    bool isLogApiResponce_ = false;
};

} // namespace wsnet
