#pragma once

#include "../ip_address.h"
#include "../firewallfilter.h"
#include "dns_resolver.h"
#include "ip_routes.h"

class RoutesManager
{
public:
	RoutesManager(FirewallFilter &firewallFilter);
	~RoutesManager();

	void enable(const MIB_IPFORWARDROW &rowDefault);
	void disable();
	void setSettings(bool isExclude, const std::vector<IpAddress> &ips, const std::vector<std::string> &hosts);

private:
	FirewallFilter &firewallFilter_;
	DnsResolver dnsResolver_;
	IpRoutes ipRoutes_;

	bool isEnabled_;
	std::recursive_mutex mutex_;

	bool isExcludeMode_;

	// latest ips and hosts list
	std::vector<IpAddress> ipsLatest_;
	std::vector<std::string> hostsLatest_;

	MIB_IPFORWARDROW rowDefault_;


	void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);

};

