#pragma once

#include <string>

class MacSpoofingOnBootManager
{
public:
    MacSpoofingOnBootManager();
    ~MacSpoofingOnBootManager();
    bool setEnabled(bool bEnabled, const std::string &interface, const std::string &macAddress, bool robustMethod);

private:
    bool enable(const std::string &interface, const std::string &macAddress, bool robustMethod);
    bool disable();
};