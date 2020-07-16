#include "../all_headers.h"
#include "ip_routes.h"
#include "../logger.h"

IpRoutes::IpRoutes()
{
}


void IpRoutes::setIps(const MIB_IPFORWARDROW &rowDefault, const std::vector<IpAddress> &ips)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	// exclude duplicates
	std::set<IpAddress> ipsSet;
	for (auto ip = ips.begin(); ip != ips.end(); ++ip)
	{
		ipsSet.insert(*ip);
	}

	// find route which need delete
	std::set<IpAddress> ipsDelete;
	for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it)
	{
		if (ipsSet.find(it->first) == ipsSet.end())
		{
			ipsDelete.insert(it->first);
		}
	}

	// delete routes
	for (auto ip = ipsDelete.begin(); ip != ipsDelete.end(); ++ip)
	{
		auto fr = activeRoutes_.find(*ip);
		if (fr != activeRoutes_.end())
		{
			DWORD status = DeleteIpForwardEntry(&fr->second);
			if (status != NO_ERROR)
			{
				Logger::instance().out(L"IpRoutes::enable(), DeleteIpForwardEntry failed: %d", status);
			}
			activeRoutes_.erase(fr);
		}
	}

	DWORD dwMask = IpAddress("255.255.255.255").IPv4NetworkOrder();
	for (auto ip = ipsSet.begin(); ip != ipsSet.end(); ++ip)
	{
		auto ar = activeRoutes_.find(*ip);
		if (ar != activeRoutes_.end())
		{
			continue;
		}
		else
		{
			// add route
			MIB_IPFORWARDROW row = rowDefault;

			row.dwForwardDest = ip->IPv4NetworkOrder();
			row.dwForwardMask = dwMask;
			row.dwForwardAge = INFINITE;

			DWORD status = CreateIpForwardEntry(&row);
			if (status != NO_ERROR)
			{
				Logger::instance().out(L"IpRoutes::enable(), CreateIpForwardEntry failed: %d", status);
			}

			activeRoutes_[*ip] = row;
		}
	}
}

void IpRoutes::clear()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it)
	{
		DWORD status = DeleteIpForwardEntry(&it->second);
		if (status != NO_ERROR)
		{
			Logger::instance().out(L"IpRoutes::enable(), DeleteIpForwardEntry failed: %d", status);
		}
	}
	activeRoutes_.clear();
}
