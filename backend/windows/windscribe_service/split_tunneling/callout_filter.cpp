#include "../all_headers.h"
#include "callout_filter.h"
#include "../ip_address/ip4_address_and_mask.h"
#include "../logger.h"
#include "../utils.h"

DEFINE_GUID(
    WINDSCRIBE_NON_TCP_CALLOUT_GUID,
    0x5DF29179,
    0x344E,
    0x4F9C,
    0xA4, 0x5D, 0xC3, 0x0F, 0x95, 0x9B, 0x01, 0x2D
);

DEFINE_GUID(
    WINDSCRIBE_TCP_CALLOUT_GUID,
    0xB53C4ADE,
    0x7A35,
    0x4A63,
    0xB6, 0x90, 0xD9, 0x6B, 0xD4, 0x26, 0x23, 0x03
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


typedef struct WINDSCRIBE_CALLOUT_DATA_
{
    UINT32 localIp;
    UINT8 isExclude;
    UINT8 isAllowLanTraffic;
} WINDSCRIBE_CALLOUT_DATA;


CalloutFilter::CalloutFilter(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper), isEnabled_(false), prevIp_(0)
{
}

void CalloutFilter::enable(UINT32 ip, const AppsIds &appsIds, bool isExclude, bool isAllowLanTraffic)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    Logger::instance().out(L"CalloutFilter::enable()");

    // nothing todo if nothing changed
    if (isEnabled_ && ip == prevIp_ && appsIds == appsIds_)
    {
        return;
    }

    if (isEnabled_)
    {
        Logger::instance().out(L"CalloutFilter: disable before enable");
        disable();
    }

    appsIds_ = appsIds;
    prevIp_ = ip;

    HANDLE hEngine = fwmpWrapper_.getHandleAndLock();
    fwmpWrapper_.beginTransaction();

    bool success = true;

    if (!addProviderContext(hEngine, CALLOUT_PROVIDER_CONTEXT_IP_GUID, ip, isExclude, isAllowLanTraffic))
    {
        Logger::instance().out(L"CalloutFilter::enable(), addProviderContext failed");
        success = false;
    }

    if (success && !addCallouts(hEngine))
    {
        Logger::instance().out(L"CalloutFilter::enable(), FwpmCalloutAdd failed");
        success = false;
    }

    if (success && !addSubLayer(hEngine))
    {
        Logger::instance().out(L"CalloutFilter::enable(), addSubLayer failed");
        success = false;
    }

    if (success && !addFilters(hEngine))
    {
        Logger::instance().out(L"CalloutFilter::enable(), addFilters failed");
        success = false;
    }

    fwmpWrapper_.endTransaction();
    fwmpWrapper_.unlock();

    if (success)
    {
        isEnabled_ = true;
    }
}

void CalloutFilter::disable()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (!isEnabled_)
    {
        return;
    }
    Logger::instance().out(L"CalloutFilter::disable()");
    removeAllFilters(fwmpWrapper_);
    isEnabled_ = false;
}

bool CalloutFilter::addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 ip, bool isExclude, bool isAllowLanTraffic)
{
    bool bRet = true;
    WINDSCRIBE_CALLOUT_DATA proxyData;
    FWP_BYTE_BLOB         byteBlob = { 0 };
    proxyData.localIp = ip;
    proxyData.isExclude = isExclude;
    proxyData.isAllowLanTraffic = isAllowLanTraffic;

    FWPM_PROVIDER_CONTEXT1 providerContext = { 0 };
    UINT64 providerContextId;

    providerContext.displayData.name = (wchar_t*)L"Windscribe provider context for callout driver";
    providerContext.providerContextKey = guid;
    providerContext.type = FWPM_GENERAL_CONTEXT;

    providerContext.dataBuffer = &byteBlob;
    providerContext.dataBuffer->size = sizeof(proxyData);
    providerContext.dataBuffer->data = (UINT8*)&proxyData;

    DWORD ret = FwpmProviderContextAdd1(engineHandle, &providerContext, 0, &providerContextId);
    if (ret != ERROR_SUCCESS)
    {
        Logger::instance().out(L"CalloutFilter::addProviderContext(): %u", ret);
        bRet = false;
    }

    return bRet;
}

bool CalloutFilter::addCallouts(HANDLE engineHandle)
{
    // Callout for non-TCP traffic
    {
        FWPM_CALLOUT0 callout = { 0 };
        UINT32 calloutId;
        callout.calloutKey = WINDSCRIBE_NON_TCP_CALLOUT_GUID;
        callout.displayData.name = (wchar_t*)L"Windscribe split tunnel non-TCP callout";
        callout.applicableLayer = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
        callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
        DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
        if (ret != ERROR_SUCCESS)
        {
            Logger::instance().out(L"CalloutFilter::addCallouts(), FwpmCalloutAdd (non-TCP) failed");
            return false;
        }
    }

    // Callout for TCP traffic
    {
        FWPM_CALLOUT0 callout = { 0 };
        UINT32 calloutId;
        callout.calloutKey = WINDSCRIBE_TCP_CALLOUT_GUID;
        callout.displayData.name = (wchar_t*)L"Windscribe split tunnel TCP callout";
        callout.applicableLayer = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
        callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
        DWORD ret = FwpmCalloutAdd(engineHandle, &callout, NULL, &calloutId);
        if (ret != ERROR_SUCCESS)
        {
            Logger::instance().out(L"CalloutFilter::addCallouts(), FwpmCalloutAdd (TCP) failed");
            return false;
        }
    }
}

bool CalloutFilter::addSubLayer(HANDLE engineHandle)
{
    FWPM_SUBLAYER subLayer = { 0 };
    subLayer.subLayerKey = SUBLAYER_CALLOUT_GUID;
    subLayer.displayData.name = (wchar_t*)L"Windscribe sublayer for callout driver";
    subLayer.weight = 0x200;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd(engineHandle, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS)
    {
        Logger::instance().out(L"CalloutFilter::addSubLayer(), FwpmSubLayerAdd failed");
        return false;
    }
    return true;
}

bool CalloutFilter::addFilters(HANDLE engineHandle)
{
    bool retValue = true;

    if (appsIds_.count() == 0)
    {
        return true;
    }

    // Non-TCP traffic goes to BIND filter
    {
        std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count() + 1);
        for (size_t i = 0; i < appsIds_.count(); ++i)
        {
            conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
            conditions[i].matchType = FWP_MATCH_EQUAL;
            conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
            conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
        }
        conditions[appsIds_.count()].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
        conditions[appsIds_.count()].matchType = FWP_MATCH_NOT_EQUAL;
        conditions[appsIds_.count()].conditionValue.type = FWP_UINT8;
        conditions[appsIds_.count()].conditionValue.uint8 = IPPROTO_TCP;

        FWPM_FILTER filter = { 0 };
        filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
        filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
        filter.displayData.name = (wchar_t *)L"Windscribe non-TCP filter for callout driver";
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
        filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
        filter.numFilterConditions = static_cast<UINT32>(conditions.size());
        filter.filterCondition = &conditions[0];
        filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
        filter.action.calloutKey = WINDSCRIBE_NON_TCP_CALLOUT_GUID;

        UINT64 filterId;
        DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
        retValue = (ret == ERROR_SUCCESS);
        if (!retValue) {
            Logger::instance().out(L"CalloutFilter::addFilter(), non-TCP filter failed: %u", ret);
            return retValue;
        }
    }

    // TCP traffic goes to CONNECT filter
    {
        std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count() + 1);
        for (size_t i = 0; i < appsIds_.count(); ++i)
        {
            conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
            conditions[i].matchType = FWP_MATCH_EQUAL;
            conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
            conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
        }
        conditions[appsIds_.count()].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
        conditions[appsIds_.count()].matchType = FWP_MATCH_EQUAL;
        conditions[appsIds_.count()].conditionValue.type = FWP_UINT8;
        conditions[appsIds_.count()].conditionValue.uint8 = IPPROTO_TCP;

        FWPM_FILTER filter = { 0 };
        filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
        filter.layerKey = FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
        filter.displayData.name = (wchar_t *)L"Windscribe TCP filter for callout driver";
        filter.weight.type = FWP_UINT8;
        filter.weight.uint8 = 0x00;
        filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
        filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
        filter.numFilterConditions = static_cast<UINT32>(conditions.size());
        filter.filterCondition = &conditions[0];
        filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
        filter.action.calloutKey = WINDSCRIBE_TCP_CALLOUT_GUID;

        UINT64 filterId;
        DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
        retValue = (ret == ERROR_SUCCESS);
        if (!retValue) {
            Logger::instance().out(L"CalloutFilter::addFilter(), TCP filter failed: %u", ret);
            return retValue;
        }
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

    DWORD ret = deleteSublayer(hEngine);

    ret = FwpmCalloutDeleteByKey(hEngine, &WINDSCRIBE_NON_TCP_CALLOUT_GUID);
    ret = FwpmCalloutDeleteByKey(hEngine, &WINDSCRIBE_TCP_CALLOUT_GUID);
    ret = FwpmProviderContextDeleteByKey(hEngine, &CALLOUT_PROVIDER_CONTEXT_IP_GUID);

    fwmpWrapper.endTransaction();
    fwmpWrapper.unlock();

    return true;
}