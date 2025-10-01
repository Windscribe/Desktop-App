#pragma once

#include <string>

class MacSpoofingOnBootManager
{
public:
    static MacSpoofingOnBootManager& instance()
    {
        static MacSpoofingOnBootManager msobm;
        return msobm;
    }

    bool setEnabled(bool bEnabled, const std::string &interface, const std::string &macAddress);

private:
    MacSpoofingOnBootManager();
    ~MacSpoofingOnBootManager();

    bool enable(const std::string &interface, const std::string &macAddress);
    bool disable();
};
