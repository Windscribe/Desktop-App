#pragma once

#include <string>

class FirewallOnBootManager
{
public:
    FirewallOnBootManager();
    ~FirewallOnBootManager();
    bool setEnabled(bool bEnabled);
    void setIpTable(const std::string& ipTable) { ipTable_ = ipTable; }

private:
    bool enable();
    bool disable();

    std::string ipTable_;
};
