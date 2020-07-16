#pragma once

#include "../ip_address.h"
#include "dns_resolver.h"

class IpRoutes
{
public:
	IpRoutes();

	void setIps(const MIB_IPFORWARDROW &rowDefault, const std::vector<IpAddress> &ips);
	void clear();

private:
	std::recursive_mutex mutex_;
	std::map<IpAddress, MIB_IPFORWARDROW> activeRoutes_;
};

