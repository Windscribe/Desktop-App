#include "../all_headers.h"
#include "callout_filter.h"
#include "../logger.h"
#include "../utils.h"


DEFINE_GUID(
	WINDSCRIBE_CALLOUT_GUID,
	0x5DF29179,
	0x344E,
	0x4F9C,
	0xA4, 0x5D, 0xC3, 0x0F, 0x95, 0x9B, 0x01, 0x2D
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
} WINDSCRIBE_CALLOUT_DATA;


CalloutFilter::CalloutFilter(FwpmWrapper &fwmpWrapper): fwmpWrapper_(fwmpWrapper), isEnabled_(false), prevIp_(0)
{
}

void CalloutFilter::enable(UINT32 ip, const AppsIds &appsIds)
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

	if (!addProviderContext(hEngine, CALLOUT_PROVIDER_CONTEXT_IP_GUID, ip))
	{
		Logger::instance().out(L"CalloutFilter::enable(), addProviderContext failed");
	}

	FWPM_CALLOUT0 callout = { 0 };
	UINT32 calloutId;

	callout.calloutKey = WINDSCRIBE_CALLOUT_GUID;
	callout.displayData.name = (wchar_t*)L"Windscribe split tunnel callout";
	callout.applicableLayer = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
	callout.flags |= FWPM_CALLOUT_FLAG_USES_PROVIDER_CONTEXT;
	DWORD ret = FwpmCalloutAdd(hEngine, &callout, NULL, &calloutId);
	if (ret != ERROR_SUCCESS)
	{
		Logger::instance().out(L"CalloutFilter::enable(), FwpmCalloutAdd failed");
	}

	if (!addSubLayer(hEngine))
	{
		Logger::instance().out(L"CalloutFilter::enable(), addSubLayer failed");
	}

	if (!addFilter(hEngine))
	{
		Logger::instance().out(L"CalloutFilter::enable(), addFilter failed");
	}

	fwmpWrapper_.endTransaction();
	fwmpWrapper_.unlock();

	isEnabled_ = true;
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

bool CalloutFilter::addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 ip)
{
	bool bRet = true;
	WINDSCRIBE_CALLOUT_DATA proxyData;
	FWP_BYTE_BLOB         byteBlob = { 0 };
	proxyData.localIp = ip;

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
		bRet = false;
	}

	return bRet;
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

bool CalloutFilter::addFilter(HANDLE engineHandle)
{
	bool retValue = true;

	if (appsIds_.count() == 0)
	{
		return true;
	}

	{
		std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count());
		for (size_t i = 0; i < appsIds_.count(); ++i)
		{
			conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
			conditions[i].matchType = FWP_MATCH_EQUAL;
			conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
			conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
		}

		FWPM_FILTER filter = { 0 };
		filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
		filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
		filter.displayData.name = (wchar_t *)L"Windscribe filter for callout driver";
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = 0x00;
		filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_IP_GUID;
		filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
		filter.numFilterConditions = static_cast<UINT32>(conditions.size());
		if (conditions.size() > 0)
		{
			filter.filterCondition = &conditions[0];
		}
		filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
		filter.action.calloutKey = WINDSCRIBE_CALLOUT_GUID;

		UINT64 filterId;
		DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
		retValue = (ret == ERROR_SUCCESS);
	}


	/*if (isExcludeMode_)
	{

		if (appsIds_.count() == 0)
		{
			return true;
		}

		{
			std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count());
			for (size_t i = 0; i < appsIds_.count(); ++i)
			{
				conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
				conditions[i].matchType = FWP_MATCH_EQUAL;
				conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
				conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
			}

			FWPM_FILTER filter = { 0 };
			filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
			filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
			filter.displayData.name = (wchar_t *)L"Windscribe filter for callout driver";
			filter.weight.type = FWP_UINT8;
			filter.weight.uint8 = 0x00;
			filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_DEFAULT_IP_GUID;
			filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
			filter.numFilterConditions = static_cast<UINT32>(conditions.size());
			if (conditions.size() > 0)
			{
				filter.filterCondition = &conditions[0];
			}
			filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
			filter.action.calloutKey = WINDSCRIBE_CALLOUT_GUID;

			UINT64 filterId;
			DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
			retValue = (ret == ERROR_SUCCESS);
		}
	}
	// inclusive apps mode
	else
	{
		// create first filter for redirect all apps to default interface
		{
			FWPM_FILTER filter = { 0 };

			filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
			filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
			filter.displayData.name = (wchar_t *)L"Windscribe filter for callout driver";
			filter.weight.type = FWP_UINT8;
			filter.weight.uint8 = 0x00;
			filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_DEFAULT_IP_GUID;
			filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
			filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
			filter.action.calloutKey = WINDSCRIBE_CALLOUT_GUID;

			UINT64 filterId;
			DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
			if (ret != ERROR_SUCCESS)
			{
				retValue = false;
			}
		}

		// create second filter for redirect selected apps to VPN interface
		{
			std::vector<FWPM_FILTER_CONDITION> conditions(appsIds_.count());
			for (size_t i = 0; i < appsIds_.count(); ++i)
			{
				conditions[i].fieldKey = FWPM_CONDITION_ALE_APP_ID;
				conditions[i].matchType = FWP_MATCH_EQUAL;
				conditions[i].conditionValue.type = FWP_BYTE_BLOB_TYPE;
				conditions[i].conditionValue.byteBlob = appsIds_.getAppId(i);
			}

			FWPM_FILTER filter = { 0 };

			filter.subLayerKey = SUBLAYER_CALLOUT_GUID;
			filter.layerKey = FWPM_LAYER_ALE_BIND_REDIRECT_V4;
			filter.displayData.name = (wchar_t *)L"Windscribe filter for callout driver";
			filter.weight.type = FWP_UINT8;
			filter.weight.uint8 = 0x01;
			filter.providerContextKey = CALLOUT_PROVIDER_CONTEXT_VPN_IP_GUID;
			filter.flags |= FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT;
			filter.action.type = FWP_ACTION_CALLOUT_UNKNOWN;
			filter.action.calloutKey = WINDSCRIBE_CALLOUT_GUID;
			filter.numFilterConditions = static_cast<UINT32>(conditions.size());
			if (conditions.size() > 0)
			{
				filter.filterCondition = &conditions[0];
			}

			UINT64 filterId;
			DWORD ret = FwpmFilterAdd(engineHandle, &filter, NULL, &filterId);
			if (ret != ERROR_SUCCESS)
			{
				retValue = false;
			}
		}
	}*/

	return retValue;
}

bool CalloutFilter::deleteSublayer(HANDLE engineHandle)
{
	bool b = Utils::deleteAllFiltersForSublayer(engineHandle, &SUBLAYER_CALLOUT_GUID, FWPM_LAYER_ALE_BIND_REDIRECT_V4);
	DWORD dwRet = FwpmSubLayerDeleteByKey0(engineHandle, &SUBLAYER_CALLOUT_GUID);
	return b && dwRet == NO_ERROR;
}


bool CalloutFilter::removeAllFilters(FwpmWrapper &fwmpWrapper)
{
	HANDLE hEngine = fwmpWrapper.getHandleAndLock();
	fwmpWrapper.beginTransaction();

	DWORD ret = deleteSublayer(hEngine);
	
	ret = FwpmCalloutDeleteByKey(hEngine, &WINDSCRIBE_CALLOUT_GUID);
	ret = FwpmProviderContextDeleteByKey(hEngine, &CALLOUT_PROVIDER_CONTEXT_IP_GUID);

	fwmpWrapper.endTransaction();
	fwmpWrapper.unlock();

	return true;
}