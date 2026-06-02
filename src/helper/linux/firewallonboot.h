#pragma once

#include <string>
#include <vector>

class FirewallOnBootManager
{
public:
    static FirewallOnBootManager& instance()
    {
        static FirewallOnBootManager fobm;
        return fobm;
    }

    bool setEnabled(bool enabled, bool allowLanTraffic);
    void setIpTable(const std::vector<std::string>& ipTable) { ipTable_ = ipTable; }

private:
    FirewallOnBootManager();
    ~FirewallOnBootManager();

    std::string comment_;

    bool enable(bool allowLanTraffic);
    bool disable();

    std::vector<std::string> ipTable_;
};
