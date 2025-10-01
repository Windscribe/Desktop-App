#pragma once

#include <map>
#include <string>
#include <vector>

class Ipv6Manager
{
public:
    static Ipv6Manager& instance()
    {
        static Ipv6Manager ipv6Manager;
        return ipv6Manager;
    }
    bool setEnabled(bool bEnabled);

private:
    struct IPv6State {
        std::string state;  // Manual, Automatic, Off or empty
        std::string ipAddress;
        std::string routerAddress;
        std::string prefixLength;

        bool operator!=(const IPv6State& rhs) const
        {
            return state != rhs.state || ipAddress != rhs.ipAddress || routerAddress != rhs.routerAddress || prefixLength != rhs.prefixLength;
        }
    };

    explicit Ipv6Manager();
    ~Ipv6Manager();
    std::vector<std::string> getInterfaces();
    void saveIpv6States(std::vector<std::string> &interfaces);
    IPv6State getState(std::string &interface);

    std::map<std::string, IPv6State> interfaceStates_;
};
