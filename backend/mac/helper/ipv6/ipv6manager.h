#pragma once

#include <map>
#include <string>
#include <vector>

class Ipv6Manager
{
public:
    explicit Ipv6Manager();
    ~Ipv6Manager();

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

	std::vector<const std::string> getInterfaces();
	void saveIpv6States(const std::vector<const std::string> &interfaces);
    IPv6State getState(const std::string &interface);

    bool enabled_;
    std::map<std::string, IPv6State> interfaceStates_;
};
