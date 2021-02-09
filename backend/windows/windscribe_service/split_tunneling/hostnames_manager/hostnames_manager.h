#pragma once

#include "../../ip_address/ip4_address_and_mask.h"
#include "../../firewallfilter.h"
#include "dns_resolver.h"
#include "ip_routes.h"

class HostnamesManager
{
public:
	explicit HostnamesManager(FirewallFilter &firewallFilter);
	~HostnamesManager();

	void enable(const std::string &gatewayIp, unsigned long ifIndex);
	void disable();
	void setSettings(bool isExclude, const std::vector<Ip4AddressAndMask> &ips, const std::vector<std::string> &hosts);

private:
	FirewallFilter &firewallFilter_;
	DnsResolver dnsResolver_;
	IpRoutes ipRoutes_;

	bool isEnabled_;
	std::recursive_mutex mutex_;

	bool isExcludeMode_;

	// latest ips and hosts list
	std::vector<Ip4AddressAndMask> ipsLatest_;
	std::vector<std::string> hostsLatest_;

	MIB_IPFORWARDROW rowDefault_;


	void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);

};

