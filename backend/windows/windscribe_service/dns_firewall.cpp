#include "all_headers.h"

#include "dns_firewall.h"
#include "logger.h"
#include "utils.h"
#include "ip_address.h"

#define	FIREWALL_SUBLAYER_DNS_NAMEW     L"WindscribeDnsFirewallSublayer"
#define UUID_LAYER_DNS L"7e4a5678-bc70-45e8-8674-21a8e610fd02"


DnsFirewall::DnsFirewall(FwpmWrapper &fwmpWrapper) : fwmpWrapper_(fwmpWrapper), bCurrentState_(false)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	UuidFromString((RPC_WSTR)UUID_LAYER_DNS, &subLayerGUID_);
}

DnsFirewall::~DnsFirewall()
{
	disable();
	WSACleanup();
}

void DnsFirewall::disable()
{
	if (bCurrentState_)
	{
		HANDLE hEngine = fwmpWrapper_.getHandleAndLock();

		fwmpWrapper_.beginTransaction();
		bCurrentState_ = false;

		if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_))
		{
			Logger::instance().out(L"DnsFirewall::disable(), all filters deleted");
		}
		else
		{
			Logger::instance().out(L"DnsFirewall::disable(), failed delete filters");
		}

		fwmpWrapper_.endTransaction();
		fwmpWrapper_.unlock();
	}
}

void DnsFirewall::enable()
{
	// if already enabled, first remove previous filter
	if (bCurrentState_)
	{
		disable();
	}

	HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
	fwmpWrapper_.beginTransaction();

	FWPM_SUBLAYER0 subLayer = { 0 };

	subLayer.subLayerKey = subLayerGUID_;
	subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_DNS_NAMEW;
	subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_DNS_NAMEW;
	subLayer.flags = 0;
	subLayer.weight = 0x100;

	DWORD dwFwAPiRetCode = FwpmSubLayerAdd0(hEngine, &subLayer, NULL);
	if (dwFwAPiRetCode != ERROR_SUCCESS && dwFwAPiRetCode != FWP_E_ALREADY_EXISTS)
	{
		Logger::instance().out(L"DnsFirewall::enable(), FwpmSubLayerAdd0 failed");
		fwmpWrapper_.endTransaction();
		fwmpWrapper_.unlock();
		return;
	}
	
	addFilters(hEngine);

	fwmpWrapper_.endTransaction();
	fwmpWrapper_.unlock();
	bCurrentState_ = true;
}

void DnsFirewall::addFilters(HANDLE engineHandle)
{
	std::vector<std::wstring> dnsServers = getDnsServers();
	// add block filter for DNS ips for protocol UDP port 53
	for (size_t i = 0; i < dnsServers.size(); ++i)
	{
		std::vector<FWPM_FILTER_CONDITION0> condition(2);
		FWP_V4_ADDR_AND_MASK addrMask;
		memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 2);

		DWORD dwFwAPiRetCode;
		FWPM_FILTER0 filter = { 0 };

		filter.subLayerKey = subLayerGUID_;
		filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_DNS_NAMEW;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		filter.action.type = FWP_ACTION_BLOCK;
		filter.flags = 0;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x00;
		filter.filterCondition = &condition[0];
		filter.numFilterConditions = 2;

		condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		condition[0].matchType = FWP_MATCH_EQUAL;
		condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
		condition[0].conditionValue.v4AddrMask = &addrMask;

		IpAddress ipAddress(dnsServers[i].c_str());
		addrMask.addr = ipAddress.IPv4HostOrder();
		addrMask.mask = VISTA_SUBNET_MASK;

		condition[1].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
		condition[1].matchType = FWP_MATCH_EQUAL;
		condition[1].conditionValue.type = FWP_UINT16;
		condition[1].conditionValue.uint16 = 53;

		UINT64 filterId;
		dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
		if (dwFwAPiRetCode != ERROR_SUCCESS)
		{
			Logger::instance().out(L"DnsFirewall::addFilters(), FwpmFilterAdd0 failed");
		}
		else
		{
			Logger::instance().out(L"added dns filter for %s", dnsServers[i].c_str());
		}
	}
}

std::vector<std::wstring> DnsFirewall::getDnsServers()
{
	std::vector<std::wstring> dnsServers;

	DWORD dwRetVal = 0;
	ULONG outBufLen = 0;
	ULONG iterations = 0;
	const int MAX_TRIES = 5;
	std::vector<unsigned char> arr;
	arr.resize(1);

	outBufLen = arr.size();
	do {
		dwRetVal = GetAdaptersAddresses(AF_UNSPEC, NULL, NULL, (PIP_ADAPTER_ADDRESSES)&arr[0], &outBufLen);
		if (dwRetVal == ERROR_BUFFER_OVERFLOW)
		{
			arr.resize(outBufLen);
		}
		else
		{
			break;
		}
		iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));

	if (dwRetVal != NO_ERROR)
	{
		return dnsServers;
	}

	PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)&arr[0];
	while (pCurrAddresses)
	{
		if (wcsstr(pCurrAddresses->Description, L"Windscribe") == 0) // ignore Windscribe TAP and Windscribe Ikev2 adapters
		{
			PIP_ADAPTER_DNS_SERVER_ADDRESS dnsServerAddress = pCurrAddresses->FirstDnsServerAddress;
			while (dnsServerAddress)
			{
				if (dnsServerAddress->Address.lpSockaddr->sa_family == AF_INET)
				{
					const int BUF_LEN = 256;
					WCHAR szBuf[BUF_LEN];
					DWORD bufSize = BUF_LEN;
					int ret = WSAAddressToString(dnsServerAddress->Address.lpSockaddr, dnsServerAddress->Address.iSockaddrLength, NULL, szBuf, &bufSize);
					if (ret == 0)  // ok
					{
						dnsServers.push_back(std::wstring(szBuf));
					}
				}
				dnsServerAddress = dnsServerAddress->Next;
			}
		}

		pCurrAddresses = pCurrAddresses->Next;
	}

	return dnsServers;
}