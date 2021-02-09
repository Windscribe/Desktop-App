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

	void setSettings(bool isEnabled, bool isExclude, const std::vector<std::wstring> &apps, const std::vector<std::wstring> &ips, const std::vector<std::string> &hosts);
	void setConnectStatus(CMD_CONNECT_STATUS &connectStatus);
    void setKeepLocalSocketsOnDisconnect(bool value) { bKeepLocalSockets_ = value; }

	static void removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	FirewallFilter &firewallFilter_;
	CalloutFilter calloutFilter_;
	RoutesManager routesManager_;
	HostnamesManager hostnamesManager_;
	SplitTunnelServiceManager splitTunnelServiceManager_;

    bool bKeepLocalSockets_;
	AppsIds windscribeExecutablesIds_;
	CMD_CONNECT_STATUS connectStatus_;
	bool isSplitTunnelEnabled_;
	bool isExclude_;
	std::vector<std::wstring> apps_;
	bool prevIsSplitTunnelActive_;
	bool prevIsExclude_;

	void detectWindscribeExecutables();
	void updateState();
};

