#pragma once

#include <string>
#include <vector>
#include "../../posix_common/helper_commands.h"

class FirewallController
{
public:
    inline static const std::string kTag = "Windscribe client rule";

    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    bool enable(bool ipv6, const std::string &rules);
    void disable();
    bool enabled(const std::string &tag = kTag);
    void getRules(bool ipv6, std::string *outRules);

    void setSplitTunnelingEnabled(CMD_SEND_CONNECT_STATUS connectStatus_, bool isEnabled, bool isExclude);
    void setSplitTunnelIpExceptions(const std::vector<std::string> &ips);

private:
    FirewallController();
    ~FirewallController();

    CMD_SEND_CONNECT_STATUS connectStatus_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<std::string> splitTunnelIps_;
    std::string prevAdapter_;

    void removeExclusiveIpRules();
    void removeInclusiveIpRules();
    void removeExclusiveAppRules();
    void removeInclusiveAppRules();
    void setSplitTunnelAppExceptions();
    void setSplitTunnelIngressRules();
    void addRule(const std::vector<std::string> &args);
};
