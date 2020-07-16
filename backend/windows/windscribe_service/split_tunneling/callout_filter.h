#pragma once

#include "../apps_ids.h"
#include "../fwpm_wrapper.h"

class CalloutFilter
{
public:
	CalloutFilter(FwpmWrapper &fwmpWrapper);

	void enable(UINT32 ipTap, UINT32 ipDefault);
	void disable();
	void setSettings(bool isExclude, const AppsIds &appsIds);

	static bool removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
	bool addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 ip);
	bool addSubLayer(HANDLE engineHandle);
	bool addFilter(HANDLE engineHandle);

	static bool deleteSublayer(HANDLE engineHandle);

	FwpmWrapper &fwmpWrapper_;

	bool isEnabled_;
	std::recursive_mutex mutex_;

	AppsIds appsIds_;
	bool isExcludeMode_;

	// latest apps list
	//std::vector<std::wstring> appsLatest_;

};

