#pragma once

#include "../ip_address/ip4_address_and_mask.h"
#include "dns_resolver.h"

class IpRoutes
{
public:
	IpRoutes();

	void setIps(const MIB_IPFORWARDROW &rowDefault, const std::vector<Ip4AddressAndMask> &ips);
	void clear();

private:
	std::recursive_mutex mutex_;
	std::map<Ip4AddressAndMask, MIB_IPFORWARDROW> activeRoutes_;
};

