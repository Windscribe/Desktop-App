#pragma once

#include <string>

class FirewallOnBootManager
{
public:
    FirewallOnBootManager();
    ~FirewallOnBootManager();
    bool setEnabled(bool bEnabled, const std::string &rules);

private:
    bool enable(const std::string &rules);
    bool disable();
};