#pragma once

#include "callout_filter.h"
#include "routes_manager.h"
#include "split_tunnel_service_manager.h"
#include "hostnames_manager/hostnames_manager.h"
#include "../apps_ids.h"
#include "../firewallfilter.h"
#include "../ipc/servicecommunication.h"

class SplitTunneling
{
public:
	SplitTunneling(FirewallFilter &firewallFilter, FwpmWrapper &fwmpWrapper);
	~SplitTunneling();

	//void start();
	//void stop();
	void setSettings(bool isActive, bool isExclude, const std::vector<std::wstring> &apps, const std::vector<std::wstring> &ips, const std::vector<std::string> &hosts);
	void setConnectStatus(CMD_CONNECT_STATUS &connectStatus);
    void setKeepLocalSocketsOnDisconnect(bool value) { bKeepLocalSockets_ = value; }

	static void removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	FirewallFilter &firewallFilter_;
	CalloutFilter calloutFilter_;
	RoutesManager routesManager_;
	HostnamesManager hostnamesManager_;
	SplitTunnelServiceManager splitTunnelServiceManager_;

	/*bool bStarted_;
	bool bTapConnected_;*/
    bool bKeepLocalSockets_;
	AppsIds windscribeExecutablesIds_;

	/*struct ROUTE_ITEM
	{
		IF_INDEX interfaceIndex;
		DWORD metric;
		MIB_IPFORWARDROW row;
	};*/
	CMD_CONNECT_STATUS connectStatus_;
	bool isSplitTunnelActive_;
	bool isExclude_;
	std::vector<std::wstring> apps_;

	void detectWindscribeExecutables();

	void updateState();
};

