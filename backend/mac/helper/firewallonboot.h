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

    bool setEnabled(bool bEnabled);
    void setIpTable(const std::string& ipTable) { ipTable_ = ipTable; }

private:
    FirewallOnBootManager();
    ~FirewallOnBootManager();

    bool enable();
    bool disable();

    std::string ipTable_;
};
