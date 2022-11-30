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
	std::vector<const std::string> getInterfaces();
	void saveIpv6States(const std::vector<const std::string> &interfaces);
	std::string getState(const std::string &interface);

    bool enabled_;
    std::map<std::string, std::string> interfaceStates_;
};