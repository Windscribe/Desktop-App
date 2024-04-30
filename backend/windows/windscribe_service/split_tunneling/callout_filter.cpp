#include "../all_headers.h"
#include "callout_filter.h"
#include "../adapters_info.h"
#include "../ip_address/ip4_address_and_mask.h"
#include "../logger.h"
#include "../utils.h"

DEFINE_GUID(
    WINDSCRIBE_CALLOUT_GUID,
    0xB53C4ADE,
    0x7A35,
    0x4A63,
    0xB6, 0x90, 0xD9, 0x6B, 0xD4, 0x26, 0x21, 0x03
);

DEFINE_GUID(
    CALLOUT_PROVIDER_CONTEXT_IP_GUID,
    0x9DCD5EC9,
    0x56A2,
    0x42D3,
    0xA1, 0x8A, 0x52, 0xA7, 0x99, 0xA1, 0xD4, 0x62
);

DEFINE_GUID(
    SUBLAYER_CALLOUT_GUID,
    0xB32BE898,
    0x4077,
    0x45E2,
    0xBC, 0x77, 0x1F, 0xA4, 0x6E, 0x99, 0x4A, 0x4B
);

#pragma pack(push,1)
typedef struct WINDSCRIBE_CALLOUT_DATA_
{
    UINT32 localIp;
    UINT32 vpnIp;
    UINT8 isExclude;

    // this is a list of addresses represented by pairs of addreses and masks:
    // excludeAddresses[0] -> Address_0; excludeAddresses[1] -> Mask_0
    // .....
    // excludeAddresses[cntExcludeAddresses - 2] -> Address_last; excludeAddresses[cntExcludeAddresses - 1] -> Mask_last
    UINT16 cntExcludeAddresses;
    UINT32 excludeAddresses[1];

} WINDSCRIBE_CALLOUT_DATA;

#pragma pack(pop)


CalloutFilter::CalloutFilter(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper), isEnabled_(false), prevLocalIp_(0), prevVpnIp_(0)
{
}

void CalloutFilter::enable(UINT32 localIp, UINT32 vpnIp, const AppsIds &appsIds, bool isExclude, bool isAllowLanTraffic)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    Logger::instance().out(L"CalloutFilter::enable()");

    // nothing todo if nothing changed
    if (isEnabled_&&
        localIp == prevLocalIp_ &&
        vpnIp == prevVpnIp_ &&
        appsIds == appsIds_ &&
        isExclude == prevIsExclude_ &&
        isAllowLanTraffic == prevIsAllowLanTraffic_)
    {
        return;
    }

    if (isEnabled_) {
        Logger::instance().out(L"CalloutFilter: disable before enable");
        disable();
    }

    appsIds_ = appsIds;
    prevLocalIp_ = localIp;
    prevVpnIp_ = vpnIp;
    prevIsExclude_ = isExclude;
    prevIsAllowLanTraffic_ = isAllowLanTraffic;

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    bool success = true;

    if (!addProviderContext(hEngine, CALLOUT_PROVIDER_CONTEXT_IP_GUID, localIp, vpnIp, isExclude)) {
        Logger::instance().out(L"CalloutFilter::enable(), addProviderContext failed");
        success = false;
    }

    if (success && !addCallouts(hEngine)) {
        Logger::instance().out(L"CalloutFilter::enable(), FwpmCalloutAdd failed");
        success = false;
    }

    if (success && !addSubLayer(hEngine)) {
        Logger::instance().out(L"CalloutFilter::enable(), addSubLayer failed");
        success = false;
    }

    if (success && !addFilters(hEngine, !isExclude && isAllowLanTraffic, appsIds)) {
        Logger::instance().out(L"CalloutFilter::enable(), addFilters failed");
        success = false;
    }

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();

    if (success) {
        isEnabled_ = true;
    }
}

void CalloutFilter::disable()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    Logger::instance().out(L"CalloutFilter::disable()");
    removeAllFilters(fwmpWrapper_);
    isEnabled_ = false;
}

bool CalloutFilter::addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 localIp, UINT32 vpnIp, bool isExclude)
{
    const std::vector<std::string> lanRanges = {
        "10.0.0.0/9",    // this and the following 10.x.x.x ranges represent the range 10.0.0.0 - 10.254.255.255,
        "10.128.0.0/10", // specifically excluding the reserved range of 10.255.255.0/24
        "10.192.0.0/11",
        "10.224.0.0/12",
        "10.240.0.0/13",
        "10.248.0.0/14",
        "10.252.0.0/15",
        "10.254.0.0/16",
        "127.0.0.0/8",
        "169.254.0.0/16",
        "172.16.0.0/12",
        "192.168.0.0/16",
        "224.0.0.0/4",
    };

    bool bRet = true;
    FWP_BYTE_BLOB         byteBlob = { 0 };

    // fill callout data struct
    // here is some C style code needed to fill the structure and pass it to the driver
    std::vector<unsigned char> data;
    data.resize(sizeof(WINDSCRIBE_CALLOUT_DATA) - sizeof(WINDSCRIBE_CALLOUT_DATA().excludeAddresses) + lanRanges.size() * 2 * sizeof(UINT32));
    WINDSCRIBE_CALLOUT_DATA *proxyData = reinterpret_cast<WINDSCRIBE_CALLOUT_DATA *>(data.data());

    proxyData->localIp = localIp;
    proxyData->vpnIp = vpnIp;
    proxyData->isExclude = isExclude;
    proxyData->cntExcludeAddresses = lanRanges.size() * 2;

    int ind = 0;
    for (const auto &it: lanRanges) {
        Ip4AddressAndMask addr(it.c_str());
        proxyData->excludeAddresses[ind++] = addr.ipHostOrder();
        proxyData->excludeAddresses[ind++] = addr.maskHostOrder();
    }

    FWPM_PROVIDER_CONTEXT1 providerContext = { 0 };
    UINT64 providerContextId;

    providerContext.displayData.name = (wchar_t*)L"Windscribe provider context for callout driver";
    providerContext.providerContextKey = guid;
    providerContext.type = FWPM_GENERAL_CONTEXT;

    providerContext.dataBuffer = &byteBlob;
    providerContext.dataBuffer->size = data.size();
    providerContext.dataBuffer->data = (UINT8*)data.data();

    DWORD ret = FwpmProviderContextAdd1(engineHandle, &providerContext, 0, &providerContextId);
    if (ret != ERROR_SUCCESS) {
        Logger::instance().out(L"CalloutFilter::addProviderContext(): %u", ret);
        bRet = false;
    }

    return bRet;
}

bool CalloutFilter::addCallouts(HANDLE engineHandle)
{
    FWPM_CALLOUT0 callout = { 0 };
    UINT32 calloutId;
    callout.calloutKey = WINDSCRIBE_CALLOUT_GUID;
    callout.displayData.name = (wchar_t*)L"Windscribe split tunnel callout";
    callout.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
    callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
    DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
    if (ret != ERROR_SUCCESS) {
        Logger::instance().out(L"CalloutFilter::addCallouts(), FwpmCalloutAdd failed");
        return false;
    }
    return true;
}

bool CalloutFilter::addSubLayer(HANDLE engineHandle)
{
    FWPM_SUBLAYER subLayer = { 0 };
    subLayer.subLayerKey = SUBLAYER_CALLOUT_GUID;
    subLayer.displayData.name = (wchar_t*)L"Windscribe sublayer for callout driver";
    subLayer.weight = 0x200;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd(engineHandle, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS) {
        Logger::instance().out(L"CalloutFilter::addSubLayer(), FwpmSubLayerAdd failed");
        return false;
    }
    return true;
}

bool CalloutFilter::addFilters(HANDLE engineHandle, bool withTcpFilters, const AppsIds &appsIds)
{
    bool retValue = true;

    if (appsIds.count() == 0) {
        return true;
    }

    // All traffic for apps goes to ALE_CONNECT_REDIRECT_V4 filter
    std::vector<FWPM_FILTER_CONDITION> conditions;
    conditions.reserve(appsIds.count());

    // Must match one of the apps
    for (size_t i = 0; i < appsIds.count(); ++i) {
        FWPM_FILTER_CONDITION condition;
        condition.fieldKey = FWPM_CONDITION_ALE_APP_ID;
        condition.matchType = FWP_MATCH_EQUAL;
        condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
        condition.conditionValue.byteBlob = (FWP_BYTE_BLOB *)appsIds.getAppId(i);
        conditions.push_back(condition);
    }


    FWPM_FILTER filter = { 0 };
    filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
    filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
    filter.displayData.name = (wchar_t *)L"Windscribe ALE_CONNECT_REDIRECT_V4 filter for callout driver";
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = 0x00;
    filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
    filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
    filter.numFilterConditions = static_cast<UINT32>(conditions.size());
    filter.filterCondition = &conditions[0];
    filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
    filter.action.calloutKey = WINDSCRIBE_CALLOUT_GUID;

    UINT64 filterId;
    DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
    retValue = (ret == ERROR_SUCCESS);
    if (!retValue) {
        Logger::instance().out(L"CalloutFilter::addFilter(), ALE_CONNECT_REDIRECT_V4 filter failed: %u", ret);
        return retValue;
    }

    return retValue;
}

bool CalloutFilter::deleteSublayer(HANDLE engineHandle)
{
    bool b1 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_BIND_REDIRECT_V4);
    bool b2 = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_CONNECT_REDIRECT_V4);
    DWORD dwRet = FwpmSubLayerDeleteByKey0(engineHandle, &SUBLAYER_CALLOUT_GUID);
    return b1 && b2 && dwRet == NO_ERROR;
}


bool CalloutFilter::removeAllFilters(FwpmWrapper &fwmpWrapper)
{
    HANDLE hEngine = fwmpWrapper.getHandleAndLock();
    fwmpWrapper.beginTransaction();

    deleteSublayer(hEngine);

    FwpmCalloutDeleteByKey(hEngine, &WINDSCRIBE_CALLOUT_GUID);
    FwpmProviderContextDeleteByKey(hEngine, &CALLOUT_PROVIDER_CONTEXT_IP_GUID);

    fwmpWrapper.endTransaction();
    fwmpWrapper.unlock();

    return true;
}
