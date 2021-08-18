#pragma once
#include "fwpm_wrapper.h"

class DnsFirewall
{
public:
	explicit DnsFirewall(FwpmWrapper &fwmpWrapper);
	~DnsFirewall();

	void disable();
	void enable();

private:
	bool bCurrentState_;
	GUID subLayerGUID_;
	FwpmWrapper &fwmpWrapper_;

	void addFilters(HANDLE engineHandle);
	std::vector<std::wstring> getDnsServers();

};

