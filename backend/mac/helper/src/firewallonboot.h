#pragma once

#include <string>

class FirewallOnBootManager
{
public:
    FirewallOnBootManager();
    ~FirewallOnBootManager();
    bool setEnabled(bool bEnabled);

private:
    bool enable();
    bool disable();
};