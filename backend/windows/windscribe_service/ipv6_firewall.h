#pragma once
#include "fwpm_wrapper.h"

class Ipv6Firewall
{
public:
	explicit Ipv6Firewall(FwpmWrapper &fwmpWrapper);
	~Ipv6Firewall();

	void enableIPv6();
	void disableIPv6();

private:
	FwpmWrapper &fwmpWrapper_;
	GUID subLayerGUID_;
	void addFilters(HANDLE engineHandle);
};

