#ifndef FIREWALLFILTER_H
#define FIREWALLFILTER_H

#include "apps_ids.h"
#include "ip_address/ip4_address_and_mask.h"
#include "fwpm_wrapper.h"

#define UUID_LAYER L"367016b3-3af8-4966-8442-d8bb6435f4a0"
#define FIREWALL_SUBLAYER_NAMEW          L"WindscribeFirewall"

class FirewallFilter
{
public:
    explicit FirewallFilter(FwpmWrapper &fwmpWrapper);
    ~FirewallFilter();

    void on(const wchar_t *ip, bool bAllowLocalTraffic);
    void off();
    bool currentStatus();

	// split tunnelling parameters
	void setSplitTunnelingEnabled();
	void setSplitTunnelingDisabled();

	void setSplitTunnelingAppsIds(const AppsIds &appsIds, bool isExclusiveMode);
	void setSplitTunnelingWhitelistIps(const std::vector<Ip4AddressAndMask> &ips);

private:
	FwpmWrapper &fwpmWrapper_;
    GUID   subLayerGUID_;

	std::recursive_mutex mutex_;
	bool lastAllowLocalTraffic_;

	bool currentStatusImpl(HANDLE engineHandle);
	void offImpl(HANDLE engineHandle);

	void addFilters(HANDLE engineHandle, const wchar_t *ip, bool bAllowLocalTraffic);
	UINT64 addPermitFilterForAdapter(HANDLE engineHandle, NET_LUID luid, UINT8 weight);
	void addBlockFiltersForAdapter(HANDLE engineHandle, NET_LUID luid, UINT8 weight);
	UINT64 addFilterForAdapterAndIpRange(HANDLE engineHandle, FWP_ACTION_TYPE type, NET_LUID luid, Ip4AddressAndMask range, UINT8 weight);
	
	void addPermitFilterForAppsIds(HANDLE engineHandle, UINT8 weight);
	void addPermitFilterForAppsIdsExclusiveMode(HANDLE engineHandle, UINT8 weight);
	void addPermitFilterForAppsIdsInclusiveMode(HANDLE engineHandle, UINT8 weight);
	
	void addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle, UINT8 weight);
	
	void removeAppsSplitTunnelingFilter(HANDLE engineHandle);
	void removeIpsSplitTunnelingFilter(HANDLE engineHandle);
	void removeAllSplitTunnelingFilters(HANDLE engineHandle);

    std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems);
    std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);


    std::vector<std::wstring> tapAdapters_;
	
	bool isSplitTunnelingEnabled_;
	bool isSplitTunnelingExclusiveMode_;
	AppsIds appsIds_;
	std::vector<Ip4AddressAndMask> splitRoutingIps_;
	
	std::vector<UINT64> filterIdsApps_;
	std::vector<UINT64> filterIdsSplitRoutingIps_;
};

#endif // FIREWALLFILTER_H
