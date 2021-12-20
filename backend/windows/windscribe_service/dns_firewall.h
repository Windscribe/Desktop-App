#pragma once
#include "fwpm_wrapper.h"

#include <unordered_set>

class DnsFirewall
{
public:
	explicit DnsFirewall(FwpmWrapper &fwmpWrapper);
	~DnsFirewall();

	void disable();
	void enable();

	void setExcludeIps(const std::vector<std::wstring>& ips);

private:
	bool bCurrentState_;
	GUID subLayerGUID_;
	FwpmWrapper &fwmpWrapper_;
	std::unordered_set<std::wstring> excludeIps_;

	void addFilters(HANDLE engineHandle);
	std::vector<std::wstring> getDnsServers();

};

