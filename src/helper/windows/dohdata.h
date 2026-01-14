#pragma once

// Saves doh settings for Windows 11.
class DohData
{
public:
    static DohData &instance()
    {
        static DohData dohData;
        return dohData;
    }

    void disableDohSettings();
    void enableDohSettings();

private:
    DohData() = default;

    unsigned long enableAutoDoh_ = 0;

    // shows whether doh was enabled last time.
    // Default value should be true as service always first disables and only then enables doh.
    bool lastTimeDohWasEnabled_ = true;
    bool dohRegistryWasCreated_ = false;
};
