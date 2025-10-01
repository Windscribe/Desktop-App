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

    bool setEnabled(bool enabled, bool allowLanTraffic);
    void setIpTable(const std::string& ipTable) { ipTable_ = ipTable; }

private:
    FirewallOnBootManager();
    ~FirewallOnBootManager();

    std::string comment_;

    bool enable(bool allowLanTraffic);
    bool disable();

    std::string ipTable_;
};
