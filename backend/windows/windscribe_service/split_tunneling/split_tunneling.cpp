#include "../all_headers.h"
#include "split_tunneling.h"
#include "../close_tcp_connections.h"
#include "../logger.h"
#include "../utils.h"

SplitTunneling::SplitTunneling(FirewallFilter &firewallFilter, FwpmWrapper &fwmpWrapper) : firewallFilter_(firewallFilter), calloutFilter_(fwmpWrapper), 
				 hostnamesManager_(firewallFilter), isSplitTunnelEnabled_(false), isExclude_(false), prevIsSplitTunnelActive_(false), prevIsExclude_(false)
{
	connectStatus_.isConnected = false;
	detectWindscribeExecutables();
}

SplitTunneling::~SplitTunneling()
{
	assert(isSplitTunnelEnabled_ == false);
}

void SplitTunneling::setSettings(bool isEnabled, bool isExclude, const std::vector<std::wstring> &apps, const std::vector<std::wstring> &ips, const std::vector<std::string> &hosts)
{
	isSplitTunnelEnabled_ = isEnabled;
	isExclude_ = isExclude;

	apps_ = apps;

	std::vector<Ip4AddressAndMask> ipsList;
	for (auto it = ips.begin(); it != ips.end(); ++it)
	{
		ipsList.push_back(Ip4AddressAndMask(it->c_str()));
	}
	hostnamesManager_.setSettings(isExclude, ipsList, hosts);
	routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);
	updateState();	
}

void SplitTunneling::setConnectStatus(CMD_CONNECT_STATUS &connectStatus)
{
	connectStatus_ = connectStatus;
	routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);
	updateState();
}


void SplitTunneling::removeAllFilters(FwpmWrapper &fwmpWrapper)
{
	CalloutFilter::removeAllFilters(fwmpWrapper);
}

void SplitTunneling::detectWindscribeExecutables()
{
	std::wstring exePath = Utils::getExePath();
	std::wstring fileFilter = exePath + L"\\*.exe";

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFileEx(fileFilter.c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, 0);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	std::vector<std::wstring> windscribeExeFiles;
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::wstring f = exePath + L"\\" + ffd.cFileName;
			windscribeExeFiles.push_back(f);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);

	windscribeExecutablesIds_.setFromList(windscribeExeFiles);
}

void SplitTunneling::updateState()
{
	bool isSplitTunnelActive = connectStatus_.isConnected && isSplitTunnelEnabled_;

	if (isSplitTunnelActive)
	{
		splitTunnelServiceManager_.start();

		DWORD redirectIp;
		AppsIds appsIds;
		appsIds.setFromList(apps_);

		if (isExclude_)
		{
			Ip4AddressAndMask ipAddress(connectStatus_.defaultAdapter.adapterIp.c_str());
			redirectIp = ipAddress.ipNetworkOrder();
			hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp, connectStatus_.defaultAdapter.ifIndex);
		}
		else
		{
			appsIds.addFrom(windscribeExecutablesIds_);
		
			Ip4AddressAndMask ipAddress(connectStatus_.vpnAdapter.adapterIp.c_str());
			redirectIp = ipAddress.ipNetworkOrder();	
			hostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp, connectStatus_.vpnAdapter.ifIndex);
		}

		firewallFilter_.setSplitTunnelingAppsIds(appsIds, isExclude_);
		firewallFilter_.setSplitTunnelingEnabled();
		calloutFilter_.enable(redirectIp, appsIds);
	}
	else
	{
		calloutFilter_.disable();
		hostnamesManager_.disable();
		firewallFilter_.setSplitTunnelingDisabled();
		splitTunnelServiceManager_.stop();
	}

	// close TCP sockets if state changed
	if (isSplitTunnelActive != prevIsSplitTunnelActive_ && connectStatus_.isTerminateSocket)
	{
		Logger::instance().out(L"SplitTunneling::threadFunc() close TCP sockets, exclude non VPN apps");
		CloseTcpConnections::closeAllTcpConnections(connectStatus_.isKeepLocalSocket, isExclude_, apps_);
	}
	else if (isSplitTunnelActive && isExclude_ != prevIsExclude_ && connectStatus_.isTerminateSocket)
	{
		Logger::instance().out(L"SplitTunneling::threadFunc() close all TCP sockets");
		CloseTcpConnections::closeAllTcpConnections(connectStatus_.isKeepLocalSocket);
	}
	prevIsSplitTunnelActive_ = isSplitTunnelActive;
	prevIsExclude_ = isExclude_;
}