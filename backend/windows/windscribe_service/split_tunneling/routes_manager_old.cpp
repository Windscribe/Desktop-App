#include "../all_headers.h"
#include "routes_manager.h"
#include "../logger.h"

RoutesManager::RoutesManager(FirewallFilter &firewallFilter): firewallFilter_(firewallFilter), isEnabled_(false), isExcludeMode_(true)
{
	dnsResolver_.setResolveDomainsCallbackHandler(std::bind(&RoutesManager::dnsResolverCallback, this, std::placeholders::_1));
}

RoutesManager::~RoutesManager()
{
	dnsResolver_.stop();
}


void RoutesManager::enable(const MIB_IPFORWARDROW &rowDefault)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	if (!isExcludeMode_)
	{
		return;
	}

	rowDefault_ = rowDefault;
	ipRoutes_.setIps(rowDefault_, ipsLatest_);
	firewallFilter_.setSplitTunnelingWhitelistIps(ipsLatest_);
	dnsResolver_.resolveDomains(hostsLatest_);

	isEnabled_ = true;
}

void RoutesManager::disable()
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

void RoutesManager::setSettings(bool isExclude, const std::vector<Ip4AddressAndMask> &ips, const std::vector<std::string> &hosts)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	// nothing todo if nothing changed
	if (isExclude == isExcludeMode_ && ips == ipsLatest_ && hosts == hostsLatest_)
	{
		return;
	}

	ipsLatest_ = ips;
	hostsLatest_ = hosts;
	isExcludeMode_ = isExclude;

	if (isEnabled_ && isExclude)
	{
		ipRoutes_.setIps(rowDefault_, ips);
		firewallFilter_.setSplitTunnelingWhitelistIps(ips);
		dnsResolver_.resolveDomains(hosts);
	}
}

void RoutesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
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

	if (isEnabled_ && isExcludeMode_)
	{
		ipRoutes_.setIps(rowDefault_, hostsIps);
		firewallFilter_.setSplitTunnelingWhitelistIps(hostsIps);
	}
}