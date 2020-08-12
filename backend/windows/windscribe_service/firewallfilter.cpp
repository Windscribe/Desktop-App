#include "all_headers.h"

#include "firewallfilter.h"
#include "logger.h"
#include "utils.h"
#include "adapters_info.h"


FirewallFilter::FirewallFilter(FwpmWrapper &fwpmWrapper) : fwpmWrapper_(fwpmWrapper), lastAllowLocalTraffic_(false), isSplitTunnelingEnabled_(false), isSplitTunnelingExclusiveMode_(false)
{
	UuidFromString((RPC_WSTR)UUID_LAYER, &subLayerGUID_);
}

FirewallFilter::~FirewallFilter()
{
}

void FirewallFilter::on(const wchar_t *ip, bool bAllowLocalTraffic)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

	if (currentStatusImpl(hEngine) == true)
    {
		fwpmWrapper_.beginTransaction();
		offImpl(hEngine);
		fwpmWrapper_.endTransaction();
    }

	fwpmWrapper_.beginTransaction();

	FWPM_SUBLAYER0 subLayer = {0};
	subLayer.subLayerKey = subLayerGUID_;
    subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
    subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
	subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
    subLayer.weight = 0x100;

    DWORD dwRet = FwpmSubLayerAdd0(hEngine, &subLayer, NULL );
    if (dwRet == ERROR_SUCCESS)
    {
		addFilters(hEngine, ip, bAllowLocalTraffic);
    }
	else
	{
		Logger::instance().out(L"FirewallFilter::on(), FwpmSubLayerAdd0 failed");
	}

	fwpmWrapper_.endTransaction();
	fwpmWrapper_.unlock();
}

void FirewallFilter::off()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
	fwpmWrapper_.beginTransaction();
	offImpl(hEngine);
	fwpmWrapper_.endTransaction();
	fwpmWrapper_.unlock();
}

bool FirewallFilter::currentStatus()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
	bool bRet = currentStatusImpl(hEngine);
	fwpmWrapper_.unlock();
	return bRet;
}

bool FirewallFilter::currentStatusImpl(HANDLE engineHandle)
{
	FWPM_SUBLAYER0 *subLayer;
	DWORD dwRet = FwpmSubLayerGetByKey0(engineHandle, &subLayerGUID_, &subLayer);
	return dwRet == ERROR_SUCCESS;
}

void FirewallFilter::offImpl(HANDLE engineHandle)
{
	if (Utils::deleteSublayerAndAllFilters(engineHandle, &subLayerGUID_))
	{
		Logger::instance().out(L"FirewallFilter::offImpl(), all filters deleted");
	}
	else
	{
		Logger::instance().out(L"FirewallFilter::offImpl(), failed delete filters");
	}
	filterIdsApps_.clear();
	filterIdsSplitRoutingIps_.clear();
}

void FirewallFilter::setSplitTunnelingEnabled()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);

	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
	fwpmWrapper_.beginTransaction();

	removeAllSplitTunnelingFilters(hEngine);
	isSplitTunnelingEnabled_ = true;

	if (currentStatusImpl(hEngine) == true)
	{
		addPermitFilterForAppsIds(hEngine, 2);
		addPermitFilterForSplitRoutingWhitelistIps(hEngine, 2);
	}

	fwpmWrapper_.endTransaction();
	fwpmWrapper_.unlock();
}
void FirewallFilter::setSplitTunnelingDisabled()
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	
	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
	fwpmWrapper_.beginTransaction();
	removeAllSplitTunnelingFilters(hEngine);
	fwpmWrapper_.endTransaction();
	fwpmWrapper_.unlock();

	isSplitTunnelingEnabled_ = false;
}

void FirewallFilter::setSplitTunnelingAppsIds(const AppsIds &appsIds, bool isExclusiveMode)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	if (appsIds_ == appsIds && isSplitTunnelingExclusiveMode_ == isExclusiveMode)
	{
		return;
	}
	appsIds_ = appsIds;
	isSplitTunnelingExclusiveMode_ = isExclusiveMode;

	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

	// if splitunneling is ON and firewall is ON then add filter for apps immediatelly 
	if (isSplitTunnelingEnabled_ && currentStatusImpl(hEngine) == true)
	{
		fwpmWrapper_.beginTransaction();
		removeAppsSplitTunnelingFilter(hEngine);
		addPermitFilterForAppsIds(hEngine, 2);
		fwpmWrapper_.endTransaction();
	}

	fwpmWrapper_.unlock();
}

void FirewallFilter::setSplitTunnelingWhitelistIps(const std::vector<IpAddress> &ips)
{
	std::lock_guard<std::recursive_mutex> guard(mutex_);
	if (splitRoutingIps_ == ips)
	{
		return;
	}
	splitRoutingIps_ = ips;

	HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

	// if splitunneling is ON and firewall is ON then add filter for ips immediatelly 
	if (isSplitTunnelingEnabled_ && currentStatusImpl(hEngine) == true)
	{
		fwpmWrapper_.beginTransaction();
		removeIpsSplitTunnelingFilter(hEngine);
		addPermitFilterForSplitRoutingWhitelistIps(hEngine, 2);
		fwpmWrapper_.endTransaction();
	}
	fwpmWrapper_.unlock();
}

void FirewallFilter::addFilters(HANDLE engineHandle, const wchar_t *ip, bool bAllowLocalTraffic)
{
    std::vector<std::wstring> ipAddresses = split(ip, L';');

	AdaptersInfo ai;
	std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

    DWORD dwFwAPiRetCode;

    // add block filter for all IPs, for all adapters.
    {
        FWPM_FILTER0 filter = {0};

        filter.subLayerKey = subLayerGUID_;
        filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		filter.action.type = FWP_ACTION_BLOCK;
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.filterCondition = NULL;
        filter.numFilterConditions = 0;

        UINT64 filterId;
        dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
        if (dwFwAPiRetCode != ERROR_SUCCESS)
        {
            Logger::instance().out(L"Error 19");
        }
    }

    // add permit filter for TAP-adapters
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it)
    {
		addPermitFilterForAdapter(engineHandle, *it, 1);
    }

	// add permit filter for splitunneling app ids and ips
	if (isSplitTunnelingEnabled_)
	{
		addPermitFilterForAppsIds(engineHandle, 2);
		addPermitFilterForSplitRoutingWhitelistIps(engineHandle, 2);
	}


    // add block filter for all IPv6
	{
		FWPM_FILTER0 filter = { 0 };

		filter.subLayerKey = subLayerGUID_;
		filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		filter.action.type = FWP_ACTION_BLOCK;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x00;
		filter.filterCondition = NULL;
		filter.numFilterConditions = 0;

		UINT64 filterId;
		dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
		if (dwFwAPiRetCode != ERROR_SUCCESS)
		{
			Logger::instance().out(L"Error 20");
		}
    }

    // add permit filter IPv6 for TAP-adapters
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it)
    {
		Utils::addPermitFilterIPv6ForAdapter(*it, 1, (wchar_t *)FIREWALL_SUBLAYER_NAMEW, subLayerGUID_, engineHandle);
    }


    // add permit filter for specific IPs
    for (size_t i = 0; i < ipAddresses.size(); ++i)
    {
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            FWP_V4_ADDR_AND_MASK addrMask;
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x02;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_EQUAL;
            condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
            condition[0].conditionValue.v4AddrMask = &addrMask;

			IpAddress ipAddr(ipAddresses[i].c_str());
            addrMask.addr = ipAddr.IPv4HostOrder();
            addrMask.mask = VISTA_SUBNET_MASK;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 21");
            }
        }
    }

    // add permit filter (Exec("netsh advfirewall firewall add rule name=\"VPN - Out - DHCP\" dir=out action=allow protocol=UDP
    // localport=68 remoteport=67 program=\"%SystemRoot%\\system32\\svchost.exe\" service=\"dhcp\"");
    {
        FWPM_FILTER0 filter = {0};
        std::vector<FWPM_FILTER_CONDITION0> condition(2);
        memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 2);

        filter.subLayerKey = subLayerGUID_;
        filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
        filter.action.type = FWP_ACTION_PERMIT;
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x03;
        filter.filterCondition = &condition[0];
        filter.numFilterConditions = 2;

        condition[0].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
        condition[0].matchType = FWP_MATCH_EQUAL;
        condition[0].conditionValue.type = FWP_UINT16;
        condition[0].conditionValue.uint16 = 68;

        condition[1].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
        condition[1].matchType = FWP_MATCH_EQUAL;
        condition[1].conditionValue.type = FWP_UINT16;
        condition[1].conditionValue.uint16 = 67;

        UINT64 filterId;
        dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
        if (dwFwAPiRetCode != ERROR_SUCCESS)
        {
            Logger::instance().out(L"Error 22");
        }
    }

    // add permit filters for Local Network
    if (bAllowLocalTraffic)
    {
        // 192.168.0.0 - 192.168.255.255
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            FWP_V4_ADDR_AND_MASK addrMask;
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_EQUAL;
            condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
            condition[0].conditionValue.v4AddrMask = &addrMask;

			IpAddress ipAddress(L"192.168.0.0");
            addrMask.addr = ipAddress.IPv4HostOrder();
            addrMask.mask = 0xFFFF0000;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 23");
            }
        }
        // 172.16.0.0 - 172.31.255.255
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            FWP_V4_ADDR_AND_MASK addrMask;
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_EQUAL;
            condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
            condition[0].conditionValue.v4AddrMask = &addrMask;

			IpAddress ipAddress(L"172.16.0.0");
            addrMask.addr = ipAddress.IPv4HostOrder();
            addrMask.mask = 0xFFF00000;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 24");
            }
        }
        // 10.0.0.0 - 10.255.255.255
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            FWP_V4_ADDR_AND_MASK addrMask;
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_EQUAL;
            condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
            condition[0].conditionValue.v4AddrMask = &addrMask;

			IpAddress ipAddress(L"10.0.0.0");
            addrMask.addr = ipAddress.IPv4HostOrder();
            addrMask.mask = 0xFF000000;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 25");
            }
        }
        // 224.0.0.0 - 239.255.255.255 (multicast ipv4 addresses)
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            FWP_RANGE0 range;
            range.valueLow.type = FWP_UINT32;
            range.valueHigh.type = FWP_UINT32;
            range.valueLow.uint32 = 3758096384;     // 224.0.0.0
            range.valueHigh.uint32 = 4026531839;    // 239.255.255.255

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_RANGE;
            condition[0].conditionValue.type = FWP_RANGE_TYPE;
            condition[0].conditionValue.rangeValue = &range;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 26");
            }
        }
		// loopback ipv4 addresses to the local host
		{
			FWPM_FILTER0 filter = { 0 };
			std::vector<FWPM_FILTER_CONDITION0> condition(1);
			FWP_V4_ADDR_AND_MASK addrMask;
			memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

			filter.subLayerKey = subLayerGUID_;
			filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
			filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
			filter.action.type = FWP_ACTION_PERMIT;
			filter.weight.type = FWP_UINT8;
			filter.weight.uint8 = 0x04;
			filter.filterCondition = &condition[0];
			filter.numFilterConditions = 1;

			condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
			condition[0].matchType = FWP_MATCH_EQUAL;
			condition[0].conditionValue.type = FWP_V4_ADDR_MASK;
			condition[0].conditionValue.v4AddrMask = &addrMask;

			IpAddress ipAddress(L"127.0.0.0");
			addrMask.addr = ipAddress.IPv4HostOrder();
			addrMask.mask = 0xFF000000;

			UINT64 filterId;
			dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
			if (dwFwAPiRetCode != ERROR_SUCCESS)
			{
				Logger::instance().out(L"Error 25 loopback IPs");
			}
		}

        // ipv6 local addresses
		{
			FWPM_FILTER0 filter = { 0 };
			std::vector<FWPM_FILTER_CONDITION0> condition(1);
			memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

			// localhost ipv6 address
			FWP_V6_ADDR_AND_MASK ipv6;
			ZeroMemory(&ipv6, sizeof(ipv6));
			ipv6.addr[15] = 1;
			ipv6.prefixLength = 128;

			filter.subLayerKey = subLayerGUID_;
			filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
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
			dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
			if (dwFwAPiRetCode != ERROR_SUCCESS)
			{
				Logger::instance().out(L"Error 27");
			}
		}
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            FWP_RANGE0 range;
            range.valueLow.type = FWP_BYTE_ARRAY16_TYPE;
            range.valueHigh.type = FWP_BYTE_ARRAY16_TYPE;

            FWP_BYTE_ARRAY16 lowArray = { 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            range.valueLow.byteArray16 = &lowArray;
            FWP_BYTE_ARRAY16 highArray = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
            range.valueHigh.byteArray16 = &highArray;

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_RANGE;
            condition[0].conditionValue.type = FWP_RANGE_TYPE;
            condition[0].conditionValue.rangeValue = &range;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 27");
            }
        }
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            FWP_RANGE0 range;
            range.valueLow.type = FWP_BYTE_ARRAY16_TYPE;
            range.valueHigh.type = FWP_BYTE_ARRAY16_TYPE;

            FWP_BYTE_ARRAY16 lowArray = { 254, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            range.valueLow.byteArray16 = &lowArray;
            FWP_BYTE_ARRAY16 highArray = { 254, 128, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255 };
            range.valueHigh.byteArray16 = &highArray;

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_RANGE;
            condition[0].conditionValue.type = FWP_RANGE_TYPE;
            condition[0].conditionValue.rangeValue = &range;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 28");
            }
        }
        {
            FWPM_FILTER0 filter = {0};
            std::vector<FWPM_FILTER_CONDITION0> condition(1);
            memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * 1);

            FWP_RANGE0 range;
            range.valueLow.type = FWP_BYTE_ARRAY16_TYPE;
            range.valueHigh.type = FWP_BYTE_ARRAY16_TYPE;

            FWP_BYTE_ARRAY16 lowArray = { 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            range.valueLow.byteArray16 = &lowArray;
            FWP_BYTE_ARRAY16 highArray = { 254, 128, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255 };
            range.valueHigh.byteArray16 = &highArray;

            filter.subLayerKey = subLayerGUID_;
            filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
            filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
			filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
            filter.action.type = FWP_ACTION_PERMIT;
            filter.weight.type = FWP_UINT8;
            filter.weight.uint8 = 0x04;
            filter.filterCondition = &condition[0];
            filter.numFilterConditions = 1;

            condition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
            condition[0].matchType = FWP_MATCH_RANGE;
            condition[0].conditionValue.type = FWP_RANGE_TYPE;
            condition[0].conditionValue.rangeValue = &range;

            UINT64 filterId;
            dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
            if (dwFwAPiRetCode != ERROR_SUCCESS)
            {
                Logger::instance().out(L"Error 29");
            }
        }
    }
}

void FirewallFilter::addPermitFilterForAdapter(HANDLE engineHandle, NET_IFINDEX tapInd, UINT8 weight)
{
    NET_LUID luid;
    ConvertInterfaceIndexToLuid(tapInd, &luid);

    // add permit filter for TAP.
    {
        DWORD dwFwAPiRetCode;
        FWPM_FILTER0 filter = {0};
        std::vector<FWPM_FILTER_CONDITION0> condition(1);
        memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * (1));

        filter.subLayerKey = subLayerGUID_;
        filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
        filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.action.type = FWP_ACTION_PERMIT;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = weight;
        filter.filterCondition = &condition[0];
        filter.numFilterConditions = 1;

        condition[0].fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
        condition[0].matchType = FWP_MATCH_EQUAL;
        condition[0].conditionValue.type = FWP_UINT64;
        condition[0].conditionValue.uint64 = &luid.Value;

        UINT64 filterId;
        dwFwAPiRetCode = FwpmFilterAdd0(engineHandle, &filter, NULL, &filterId);
        if (dwFwAPiRetCode != ERROR_SUCCESS)
        {
            Logger::instance().out(L"Error 30");
        }
    }
}

void FirewallFilter::addPermitFilterForAppsIds(HANDLE engineHandle, UINT8 weight)
{
	if (isSplitTunnelingExclusiveMode_)
	{
		addPermitFilterForAppsIdsExclusiveMode(engineHandle, weight);
	}
	else
	{
		addPermitFilterForAppsIdsInclusiveMode(engineHandle, weight);
	}
}

void FirewallFilter::addPermitFilterForAppsIdsExclusiveMode(HANDLE engineHandle, UINT8 weight)
{
	if (appsIds_.count() == 0)
	{
		return;
	}

	std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count());
	for (size_t i = 0; i < appsIds_.count(); ++i)
	{
		conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
		conditions[i].matchType = FWP_MATCH_EQUAL;
		conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
		conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
	}

	FWPM_FILTER filter = { 0 };
	filter.subLayerKey = subLayerGUID_;
	filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
	filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
	filter.action.type = FWP_ACTION_PERMIT;
	filter.weight.type = FWP_UINT8;
	filter.weight.uint8 = 0x02;
	filter.numFilterConditions = conditions.size();
	if (conditions.size() > 0)
	{
		filter.filterCondition = &conditions[0];
	}

	UINT64 filterId;
	DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
	if (ret != ERROR_SUCCESS)
	{
		Logger::instance().out(L"Error in FirewallFilter::addPermitFilterForAppsIdsExclusiveMode");
	}
	else
	{
		filterIdsApps_.push_back(filterId);
	}
}
void FirewallFilter::addPermitFilterForAppsIdsInclusiveMode(HANDLE engineHandle, UINT8 weight)
{
	// add permit filter for all apps
	{
		FWPM_FILTER filter = { 0 };
		filter.subLayerKey = subLayerGUID_;
		filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
		filter.action.type = FWP_ACTION_PERMIT;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x02;

		UINT64 filterId;
		DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
		if (ret != ERROR_SUCCESS)
		{
			Logger::instance().out(L"Error in FirewallFilter::addPermitFilterForAppsIdsInclusiveMode, 1");
		}
		else
		{
			filterIdsApps_.push_back(filterId);
		}
	}
}

void FirewallFilter::addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle, UINT8 weight)
{
	if (!isSplitTunnelingExclusiveMode_)
	{
		return;
	}
	if (splitRoutingIps_.size() == 0)
	{
		return;
	}

	std::vector<FWPM_FILTER_CONDITION> conditions(splitRoutingIps_.size());
	std::vector<FWP_V4_ADDR_AND_MASK> addrMasks(splitRoutingIps_.size());

	for (size_t i = 0; i < splitRoutingIps_.size(); ++i)
	{
		conditions[i].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		conditions[i].matchType = FWP_MATCH_EQUAL;
		conditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
		conditions[i].conditionValue.v4AddrMask = &addrMasks[i];

		addrMasks[i].addr = splitRoutingIps_[i].IPv4HostOrder();
		addrMasks[i].mask = VISTA_SUBNET_MASK;
	}

	FWPM_FILTER filter = { 0 };
	filter.subLayerKey = subLayerGUID_;
	filter.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
	filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
	//filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
	filter.action.type = FWP_ACTION_PERMIT;
	filter.weight.type = FWP_UINT8;
	filter.weight.uint8 = 0x02;
	filter.numFilterConditions = conditions.size();
	if (conditions.size() > 0)
	{
		filter.filterCondition = &conditions[0];
	}

	UINT64 filterId;
	DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
	if (ret != ERROR_SUCCESS)
	{
		Logger::instance().out(L"Error in FirewallFilter::addPermitFilterForAppsIds");
	}
	else
	{
		filterIdsSplitRoutingIps_.push_back(filterId);
	}
}

void FirewallFilter::removeAllSplitTunnelingFilters(HANDLE engineHandle)
{
	removeAppsSplitTunnelingFilter(engineHandle);
	removeIpsSplitTunnelingFilter(engineHandle);
}
void FirewallFilter::removeAppsSplitTunnelingFilter(HANDLE engineHandle)
{
	for (size_t i = 0; i < filterIdsApps_.size(); ++i)
	{
		DWORD ret = FwpmFilterDeleteById0(engineHandle, filterIdsApps_[i]);
		if (ret != ERROR_SUCCESS)
		{
			Logger::instance().out(L"FirewallFilter::removeAllSplitTunnelingFilters(), FwpmFilterDeleteById0 failed");
		}
	}
	filterIdsApps_.clear();
}

void FirewallFilter::removeIpsSplitTunnelingFilter(HANDLE engineHandle)
{
	for (size_t i = 0; i < filterIdsSplitRoutingIps_.size(); ++i)
	{
		DWORD ret = FwpmFilterDeleteById0(engineHandle, filterIdsSplitRoutingIps_[i]);
		if (ret != ERROR_SUCCESS)
		{
			Logger::instance().out(L"FirewallFilter::removeIpsSplitTunnelingFilter(), FwpmFilterDeleteById0 failed");
		}
	}
	filterIdsSplitRoutingIps_.clear();
}


std::vector<std::wstring> &FirewallFilter::split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems) 
{
    std::wstringstream ss(s);
    std::wstring item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::wstring> FirewallFilter::split(const std::wstring &s, wchar_t delim) 
{
    std::vector<std::wstring> elems;
    split(s, delim, elems);
    return elems;
}