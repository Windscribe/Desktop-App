#include "../../all_headers.h"
#include "hostnames_manager.h"
#include "../../logger.h"

HostnamesManager::HostnamesManager(FirewallFilter &firewallFilter): firewallFilter_(firewallFilter), isEnabled_(false), isExcludeMode_(true)
{
	dnsResolver_.setResolveDomainsCallbackHandler(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1));
}

HostnamesManager::~HostnamesManager()
{
	dnsResolver_.stop();
}

void HostnamesManager::enable(const std::string &gatewayIp, unsigned long ifIndex)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	gatewayIp_ = gatewayIp;
	ifIndex_ = ifIndex;
	ipRoutes_.clear();
	ipRoutes_.setIps(gatewayIp_, ifIndex_, ipsLatest_);
	firewallFilter_.setSplitTunnelingWhitelistIps(ipsLatest_);
	dnsResolver_.cancelAll();
	dnsResolver_.resolveDomains(hostsLatest_);

	isEnabled_ = true;
}

void HostnamesManager::disable()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	if (!isEnabled_)
	{
		return;
	}
	ipRoutes_.clear();

	dnsResolver_.cancelAll();
	isEnabled_ = false; 
}

void HostnamesManager::setSettings(bool isExclude, const std::vector<Ip4AddressAndMask> &ips, const std::vector<std::string> &hosts)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	ipsLatest_ = ips;
	hostsLatest_ = hosts;
	isExcludeMode_ = isExclude;
}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	std::vector<Ip4AddressAndMask> hostsIps;
	for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it)
	{
		if (!it->second.error)
		{
			std::vector<std::string> addresses = it->second.addresses;
			for (auto addr = addresses.begin(); addr != addresses.end(); ++addr)
			{
				hostsIps.push_back(Ip4AddressAndMask(addr->c_str()));
				Logger::instance().out("RoutesManager::dnsResolverCallback(), Resolved : %s, IP: %s", it->first.c_str(), addr->c_str());
			}
		}
		else
		{
			Logger::instance().out("RoutesManager::dnsResolverCallback(), Failed resolve : %s", it->first.c_str());
		}
	}

	hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

	if (isEnabled_)
	{
		ipRoutes_.setIps(gatewayIp_, ifIndex_, hostsIps);
		firewallFilter_.setSplitTunnelingWhitelistIps(hostsIps);
	}
}