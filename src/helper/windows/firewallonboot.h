#pragma once

#include <string>

class FirewallOnBootManager
{
public:
    static FirewallOnBootManager& instance()
    {
        static FirewallOnBootManager fobm;
        return fobm;
    }

    bool setEnabled(bool enabled);
    bool isEnabled();

private:
    FirewallOnBootManager();
    ~FirewallOnBootManager();

    bool enable();
    bool disable();
};
