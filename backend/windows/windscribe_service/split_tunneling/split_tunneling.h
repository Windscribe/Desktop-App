#pragma once

#include "tap_adapter_detector.h"
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
    void setKeepLocalSocketsOnDisconnect(bool value) { bKeepLocalSockets_ = value; }

	static void removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	FirewallFilter &firewallFilter_;
	TapAdapterDetector adapterDetector_;
	CalloutFilter calloutFilter_;
	RoutesManager routesManager_;
	SplitTunnelServiceManager splitTunnelServiceManager_;
	DWORD threadId_;
	HANDLE hThread_;
	bool bStarted_;
	bool bTapConnected_;
    bool bKeepLocalSockets_;
	AppsIds windscribeExecutablesIds_;

	void callbackFunc(bool isConnected, TapAdapterDetector::TapAdapterInfo *adapterInfo);
	static DWORD WINAPI threadFunc(LPVOID n);

	struct ROUTE_ITEM
	{
		IF_INDEX interfaceIndex;
		DWORD metric;
		MIB_IPFORWARDROW row;
	};

	struct SPLIT_TUNNELING_SETTINGS
	{
		bool isExclude;
		std::vector<std::wstring> apps;
		std::vector<std::wstring> ips;
		std::vector<std::string> hosts;
	};

	bool detectDefaultInterfaceFromRouteTable(IF_INDEX excludeIfIndex, IF_INDEX &outIfIndex, MIB_IPFORWARDROW &outRow);
	bool getIpAddressDefaultInterface(IF_INDEX tapAdapterIfIndex, DWORD &outIp, MIB_IPFORWARDROW &outRow);

	void detectWindscribeExecutables();

};

