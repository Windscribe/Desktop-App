#include "../all_headers.h"
#include "split_tunneling.h"
#include "ip_address_table.h"
#include "../close_tcp_connections.h"
#include "../logger.h"
#include "../utils.h"
#include "../../../../common/utils/crashhandler.h"

#define        THREAD_MESSAGE_ADAPTER        WM_USER + 1
#define        THREAD_MESSAGE_SETTINGS       WM_USER + 2
#define        THREAD_MESSAGE_EXIT           WM_USER + 3

SplitTunneling::SplitTunneling(FirewallFilter &firewallFilter, FwpmWrapper &fwmpWrapper) : firewallFilter_(firewallFilter), calloutFilter_(fwmpWrapper), 
				routesManager_(firewallFilter), threadId_(0), hThread_(0), bStarted_(false), bTapConnected_(false), bKeepLocalSockets_(false)
{
	detectWindscribeExecutables();
}

SplitTunneling::~SplitTunneling()
{
	assert(bStarted_ == false);
}

void SplitTunneling::start()
{
	if (!bStarted_)
	{
		Logger::instance().out(L"SplitTunneling::start()");

		splitTunnelServiceManager_.start();

		bStarted_ = true;
		hThread_ = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadFunc, (LPVOID)this, 0, &threadId_);

		adapterDetector_.setCallbackHandler(std::bind(&SplitTunneling::callbackFunc, this, std::placeholders::_1, std::placeholders::_2));
		adapterDetector_.start();
	}
}

void SplitTunneling::stop()
{
	if (bStarted_)
	{
		adapterDetector_.stop();
		PostThreadMessage(threadId_, THREAD_MESSAGE_EXIT, 0, 0);
		WaitForSingleObject(hThread_, INFINITE);
		CloseHandle(hThread_);

		calloutFilter_.disable();
		routesManager_.disable();
		firewallFilter_.setSplitTunnelingDisabled();
		bStarted_ = false;

		splitTunnelServiceManager_.stop();
		Logger::instance().out(L"SplitTunneling::stop()");

		// close TCP sockets if TAP-connected
		if (bTapConnected_)
		{
			Logger::instance().out(L"SplitTunneling::threadFunc() close all TCP sockets");
			CloseTcpConnections::closeAllTcpConnections(bKeepLocalSockets_);
		}
	}
}

void SplitTunneling::setSettings(bool isExclude, const std::vector<std::wstring> &apps, const std::vector<std::wstring> &ips, const std::vector<std::string> &hosts)
{
	if (bStarted_)
	{
		SPLIT_TUNNELING_SETTINGS *ss = new SPLIT_TUNNELING_SETTINGS();
		ss->isExclude = isExclude;
		ss->apps = apps;
		ss->ips = ips;
		ss->hosts = hosts;
		PostThreadMessage(threadId_, THREAD_MESSAGE_SETTINGS, 0, (LPARAM)ss);
	}
	else
	{
		assert(false);
	}
}

void SplitTunneling::callbackFunc(bool isConnected, TapAdapterDetector::TapAdapterInfo *adapterInfo)
{
	if (bStarted_)
	{
		PostThreadMessage(threadId_, THREAD_MESSAGE_ADAPTER, isConnected, (LPARAM)adapterInfo);
	}
	else
	{
		assert(false);
	}
}


DWORD WINAPI SplitTunneling::threadFunc(LPVOID n)
{
    BIND_CRASH_HANDLER_FOR_THREAD();
	SplitTunneling *this_ = static_cast<SplitTunneling *>(n);

	MSG msg;
	while (1)
	{
		BOOL msgReturn = GetMessage(&msg, NULL, THREAD_MESSAGE_ADAPTER, THREAD_MESSAGE_EXIT);
		if (msgReturn == -1)
		{
			return 0;
		}
		else if (msgReturn)
		{
			if (msg.message == THREAD_MESSAGE_EXIT)
			{
				printf("SplitTunneling::threadFunc() exit message\n");
				return 0;
			}
			else if (msg.message == THREAD_MESSAGE_ADAPTER)
			{
				bool isConnected = msg.wParam;
				this_->bTapConnected_ = isConnected;
				TapAdapterDetector::TapAdapterInfo *adapterInfo = (TapAdapterDetector::TapAdapterInfo *)msg.lParam;

				if (isConnected)
				{
					NET_IFINDEX ifIndex;

					if (ConvertInterfaceLuidToIndex(&adapterInfo->luid, &ifIndex) == NO_ERROR)
					{
						DWORD defaultIp;
						MIB_IPFORWARDROW defaultRow;

						if (this_->getIpAddressDefaultInterface(ifIndex, defaultIp, defaultRow))
						{

							//IN_ADDR IPAddr;
							//	IPAddr.S_un.S_addr = (u_long)adapterInfo->ip;
							//IN_ADDR IPAddr2;
							//IPAddr2.S_un.S_addr = (u_long)defaultIp;
							//printf("SplitTunneling::threadFunc() Connected: %s;   default: %s\n", inet_ntoa(IPAddr), inet_ntoa(IPAddr2));

							this_->firewallFilter_.setSplitTunnelingEnabled();
							this_->calloutFilter_.enable(adapterInfo->ip, defaultIp);
							this_->routesManager_.enable(defaultRow);
						}
					}
				}
				else
				{
					this_->calloutFilter_.disable();
					this_->routesManager_.disable();
					this_->firewallFilter_.setSplitTunnelingDisabled();
					//printf("SplitTunneling::threadFunc() Disconnected\n");
				}

				if (adapterInfo)
				{
					delete adapterInfo;
				}
			}
			else if (msg.message == THREAD_MESSAGE_SETTINGS)
			{
				Logger::instance().out(L"SplitTunneling::threadFunc() set settings message");

				//firewallFilter_->set

				auto *ss = reinterpret_cast<SPLIT_TUNNELING_SETTINGS *>(msg.lParam);
				AppsIds appsIds;
				appsIds.setFromList(ss->apps);
				if (!ss->isExclude)
				{
					appsIds.addFrom(this_->windscribeExecutablesIds_);
				}

				this_->calloutFilter_.setSettings(ss->isExclude, appsIds);
				this_->firewallFilter_.setSplitTunnelingAppsIds(appsIds, ss->isExclude);
				
				std::vector<IpAddress> ips;
				for (auto it = ss->ips.begin(); it != ss->ips.end(); ++it)
				{
					ips.push_back(IpAddress(it->c_str()));
				}
				
				this_->routesManager_.setSettings(ss->isExclude, ips, ss->hosts);

				delete ss;

				// close TCP sockets if TAP-connected
				if (this_->bTapConnected_)
				{
					Logger::instance().out(L"SplitTunneling::threadFunc() close all TCP sockets");
					CloseTcpConnections::closeAllTcpConnections(this_->bKeepLocalSockets_);
				}
			}
		}
	}

	return 0;
}


bool SplitTunneling::detectDefaultInterfaceFromRouteTable(IF_INDEX excludeIfIndex, IF_INDEX &outIfIndex, MIB_IPFORWARDROW &outRow)
{
	std::vector<unsigned char> arr(sizeof(MIB_IPFORWARDTABLE));
	DWORD dwSize = 0;
	if (GetIpForwardTable((MIB_IPFORWARDTABLE *)&arr[0], &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
	{
		arr.resize(dwSize);
	}

	if (GetIpForwardTable((MIB_IPFORWARDTABLE *)&arr[0], &dwSize, 0) != NO_ERROR)
	{
		return false;
	}

	MIB_IPFORWARDTABLE *pIpForwardTable = (MIB_IPFORWARDTABLE *)&arr[0];
	std::vector<ROUTE_ITEM> items;

	for (DWORD i = 0; i < pIpForwardTable->dwNumEntries; i++)
	{
		if (pIpForwardTable->table[i].dwForwardDest == 0)
		{
			if (pIpForwardTable->table[i].dwForwardIfIndex != excludeIfIndex)
			{
				ROUTE_ITEM ri;
				ri.interfaceIndex = pIpForwardTable->table[i].dwForwardIfIndex;
				ri.metric = pIpForwardTable->table[i].dwForwardMetric1;
				ri.row = pIpForwardTable->table[i];
				items.push_back(ri);
			}
		}
	}

	if (items.size() > 0)
	{
		std::sort(items.begin(), items.end(),
			[](const ROUTE_ITEM &a, const ROUTE_ITEM &b) -> bool
		{
			return a.metric < b.metric;
		});

		outIfIndex = items[0].interfaceIndex;
		outRow = items[0].row;
		return true;
	}
	else
	{
		return false;
	}
}

bool SplitTunneling::getIpAddressDefaultInterface(IF_INDEX tapAdapterIfIndex, DWORD &outIp, MIB_IPFORWARDROW &outRow)
{
	IF_INDEX outIfIndex;
	if (detectDefaultInterfaceFromRouteTable(tapAdapterIfIndex, outIfIndex, outRow))
	{
		IpAddressTable addrTable;
		outIp = addrTable.getAdapterIpAddress(outIfIndex);
		return true;
	}

	return false;
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