#pragma once

#include <WinSock2.h>

#include <iphlpapi.h>
#include <memory>
#include <string>
#include <vector>

// wrapper for API GetAdaptersAddresses and util functions
class AdaptersInfo
{
public:
    AdaptersInfo();

    bool isAppVpnAdapter(NET_IFINDEX index) const;
    bool getIkev2AdapterInfo(NET_IFINDEX &outIfIndex, std::wstring &outIp);
    std::vector<NET_IFINDEX> getTAPAdapters();
    std::vector<std::string> getAdapterAddresses(NET_IFINDEX idx);

    // True if the adapter has at least one non-link-local (global or ULA) IPv6 address, i.e. it
    // actually carries routable IPv6. The auto-configured fe80::/10 is skipped (every v6-capable
    // adapter has one). Mirrors the macOS/Linux probeInterfaceAddresses IN6_IS_ADDR_LINKLOCAL check.
    bool hasNonLinkLocalV6(NET_IFINDEX idx) const;

private:
    std::unique_ptr< unsigned char[] > adapterInfoBuffer_;
    PIP_ADAPTER_ADDRESSES pAdapterInfo_ = NULL;

private:
    bool isAppVpnAdapter(PIP_ADAPTER_ADDRESSES ai) const;
};
