#pragma once

#include "../apps_ids.h"
#include "../fwpm_wrapper.h"

class CalloutFilter
{
public:
	explicit CalloutFilter(FwpmWrapper &fwmpWrapper);

	void enable(UINT32 ip, const AppsIds &appsIds, bool isExclude, bool allowLanTraffic);
	void disable();

	static bool removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	bool addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 ip, bool isExclude, bool isAllowLanTraffic);
	bool addCallouts(HANDLE engineHandle);
	bool addSubLayer(HANDLE engineHandle);
	bool addFilters(HANDLE engineHandle);

	static bool deleteSublayer(HANDLE engineHandle);

	FwpmWrapper &fwmpWrapper_;

	bool isEnabled_;
	std::recursive_mutex mutex_;

	AppsIds appsIds_;
	DWORD prevIp_;
};

