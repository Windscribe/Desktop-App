#include "all_headers.h"

#include "ipv6_firewall.h"
#include "logger.h"
#include "utils.h"
#include "adapters_info.h"

#define	FIREWALL_SUBLAYER_IPV6_NAMEW     L"WindscribeIPV6FirewallSublayer"
#define UUID_LAYER_IPV6 L"8b03e426-56a7-4669-a985-706406675e68"


Ipv6Firewall::Ipv6Firewall(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper)
{
	UuidFromString((RPC_WSTR)UUID_LAYER_IPV6, &subLayerGUID_);
}

Ipv6Firewall::~Ipv6Firewall()
{
	enableIPv6();
}

void Ipv6Firewall::enableIPv6()
{
	HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
	fwmpWrapper_.beginTransaction();

	if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_))
	{
		Logger::instance().out(L"Ipv6Firewall::enableIPv6(), all filters deleted");
	}
	else
	{
		Logger::instance().out(L"Ipv6Firewall::enableIPv6(), failed delete filters");
	}

	fwmpWrapper_.endTransaction();
	fwmpWrapper_.unlock();
}

void Ipv6Firewall::disableIPv6()
{
	HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
	fwmpWrapper_.beginTransaction();

	FWPM_SUBLAYER0 subLayer = { 0 };

	subLayer.subLayerKey = subLayerGUID_;
	subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
	subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
	subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
	subLayer.weight = 0x100;

	DWORD dwFwAPiRetCode = FwpmSubLayerAdd0(hEngine, &subLayer, NULL);
	if (dwFwAPiRetCode != ERROR_SUCCESS && dwFwAPiRetCode != FWP_E_ALREADY_EXISTS)
	{
		Logger::instance().out(L"Ipv6Firewall::disableIPv6(), FwpmSubLayerAdd0 failed");
		fwmpWrapper_.endTransaction();
		fwmpWrapper_.unlock();
		return;
	}
	
	addFilters(hEngine);

	fwmpWrapper_.endTransaction();
	fwmpWrapper_.unlock();
}

void Ipv6Firewall::addFilters(HANDLE engineHandle)
{
	AdaptersInfo ai;
	std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

	// add block filter for all IPv6
	{
		DWORD dwFwAPiRetCode;
		FWPM_FILTER0 filter = { 0 };

		filter.subLayerKey = subLayerGUID_;
		filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		filter.action.type = FWP_ACTION_BLOCK;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x00;
		filter.filterCondition = NULL;
		filter.numFilterConditions = 0;

		UINT64 filterId;
		dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
		if (dwFwAPiRetCode != ERROR_SUCCESS)
		{
			Logger::instance().out(L"Ipv6Firewall::addFilters(), FwpmFilterAdd0 failed");
		}
	}

	// add permit filter IPv6 for TAP-adapters
	for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it)
	{
		Utils::addPermitFilterIPv6ForAdapter(*it, 1, (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW, subLayerGUID_, engineHandle);
	}

	// add permit filter for ipv6 localhost address
	{
		FWPM_FILTER0 filter = { 0 };
		std::vector<FWPM_FILTER_CONDITION0> condition(1);
		memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

		FWP_V6_ADDR_AND_MASK ipv6;
		ZeroMemory(&ipv6, sizeof(ipv6));
		ipv6.addr[15] = 1;
		ipv6.prefixLength = 128;

		filter.subLayerKey = subLayerGUID_;
		filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		filter.action.type = FWP_ACTION_PERMIT;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x04;
		filter.filterCondition = &condition[0];
		filter.numFilterConditions = 1;

		condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		condition[0].matchType = FWP_MATCH_EQUAL;
		condition[0].conditionValue.type = FWP_V6_ADDR_MASK;
		condition[0].conditionValue.v6AddrMask = &ipv6;

		UINT64 filterId;
		DWORD dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
		if (dwFwAPiRetCode != ERROR_SUCCESS)
		{
			Logger::instance().out(L"Error 27");
		}
	}
}