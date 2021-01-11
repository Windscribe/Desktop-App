#pragma once

#include "callout_filter.h"
#include "routes_manager.h"
#include "split_tunnel_service_manager.h"
#include "../apps_ids.h"
#include "../firewallfilter.h"

class SplitTunneling
{
public:
	SplitTunneling(FirewallFilter &firewallFilter, FwpmWrapper &fwmpWrapper);
	~SplitTunneling();

	void start();
	void stop();
	void setSettings(bool isExclude, const std::vector<std::wstring> &apps, const std::vector<std::wstring> &ips, const std::vector<std::string> &hosts);
	void setConnectStatus(bool isConnected);
    void setKeepLocalSocketsOnDisconnect(bool value) { bKeepLocalSockets_ = value; }

	static void removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	FirewallFilter &firewallFilter_;
	CalloutFilter calloutFilter_;
	RoutesManager routesManager_;
	SplitTunnelServiceManager splitTunnelServiceManager_;
	bool bStarted_;
	bool bTapConnected_;
    bool bKeepLocalSockets_;
	AppsIds windscribeExecutablesIds_;

	struct ROUTE_ITEM
	{
		IF_INDEX interfaceIndex;
		DWORD metric;
		MIB_IPFORWARDROW row;
	};

	bool detectDefaultInterfaceFromRouteTable(IF_INDEX excludeIfIndex, IF_INDEX &outIfIndex, MIB_IPFORWARDROW &outRow);
	bool getIpAddressDefaultInterface(IF_INDEX tapAdapterIfIndex, DWORD &outIp, MIB_IPFORWARDROW &outRow);
	void detectWindscribeExecutables();
};

