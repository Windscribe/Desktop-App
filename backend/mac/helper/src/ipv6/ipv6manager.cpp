#include "ipv6manager.h"
#include <sstream>
#include <string>
#include <vector>
#include "utils.h"
#include "logger.h"

Ipv6Manager::Ipv6Manager() : enabled_(false)
{
}

Ipv6Manager::~Ipv6Manager()
{
    interfaceStates_.clear();
}

bool Ipv6Manager::setEnabled(bool bEnabled)
{
    if (bEnabled == enabled_) {
        LOG("IPv6 state already set");
        return false;
    }

    const std::vector<const std::string> interfaces = getInterfaces();
    if (interfaces.empty()) {
        LOG("Could not get interfaces");
        return false;
    }

    if (bEnabled) {
        LOG("Restoring IPv6");

        for (std::string interface : interfaces) {
            const std::string curState = getState(interface);
            const std::string savedState = interfaceStates_[interface];

            if ((curState != savedState && savedState == "Automatic") ||
                curState.empty() || savedState.empty() ||
                curState == "Unknown" || savedState == "Unknown") {

                Utils::executeCommand("networksetup", {"-setv6automatic", interface});
            }
        }
    } else {
        LOG("Disabling IPv6");

        saveIpv6States(interfaces);
        for (std::string interface : interfaces) {
            if (interfaceStates_[interface] != "Off") {
                Utils::executeCommand("networksetup", {"-setv6off", interface});
            }
        }
    }

    enabled_ = bEnabled;
    return true;
}

void Ipv6Manager::saveIpv6States(const std::vector<const std::string> &interfaces)
{
    for (std::string interface : interfaces) {
    	interfaceStates_[interface] = getState(interface);
    	LOG("Save state for %s: %s", interface.c_str(), interfaceStates_[interface].c_str());
    }
}

std::vector<const std::string> Ipv6Manager::getInterfaces()
{
    std::string out;
    std::vector<const std::string> interfaces;

    int status = Utils::executeCommand("networksetup", {"-listallnetworkservices"}, &out);
    if (status != 0) {
        LOG("Listing network services failed");
        return interfaces;
    }

    std::string str;
    std::stringstream ss(out);
    while (std::getline(ss, str)) {
        if (str.find("An asterisk") != std::string::npos) {
            continue;
        }
        interfaces.push_back(str);
    }

    return interfaces;
}

std::string Ipv6Manager::getState(const std::string &interface)
{
    std::string out;
	int status = Utils::executeCommand("networksetup", {"-getinfo", interface}, &out);
    if (status != 0) {
        LOG("Get network info failed");
        return "Unknown";
    }

    if (out.find("IPv6: Automatic") != std::string::npos) {
        return "Automatic";
    } else if (out.find("IPv6: Off") != std::string::npos) {
    	return "Off";
    }
    return "Unknown";
}