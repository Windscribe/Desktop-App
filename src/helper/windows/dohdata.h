#pragma once

#include <unordered_map>
#include <Windows.h>

// Saves doh settings for Windows 11.
class DohData
{
public:

    static DohData &instance()
    {
        static DohData dohData;
        return dohData;
    }

    void setEnableAutoDoh(DWORD val) { enableAutoDoh_ = val; }
    DWORD enableAutoDoh() const { return enableAutoDoh_; }

    void setLastTimeDohWasEnabled(bool val) { lastTimeDohWasEnabled_ = val; }
    bool lastTimeDohWasEnabled() const { return lastTimeDohWasEnabled_; }

    void setDohRegistryWasCreated(bool val) { dohRegistryWasCreated_ = val; }
    bool dohRegistryWasCreated() const { return dohRegistryWasCreated_; }

private:
    DohData() = default;

    INT64 enableAutoDoh_ = 0;

    // shows whether doh was enabled last time.
    // Default value should be true as service always fist disables and only then enables doh.
    bool lastTimeDohWasEnabled_ = true;
    bool dohRegistryWasCreated_ = false;
};
