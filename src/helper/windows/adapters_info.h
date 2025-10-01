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

    bool isWindscribeAdapter(NET_IFINDEX index) const;
    bool getWindscribeIkev2AdapterInfo(NET_IFINDEX &outIfIndex, std::wstring &outIp);
    std::vector<NET_IFINDEX> getTAPAdapters();
    std::vector<std::string> getAdapterAddresses(NET_IFINDEX idx);

private:
    std::unique_ptr< unsigned char[] > adapterInfoBuffer_;
    PIP_ADAPTER_ADDRESSES pAdapterInfo_ = NULL;

private:
    bool isWindscribeAdapter(PIP_ADAPTER_ADDRESSES ai) const;
};
