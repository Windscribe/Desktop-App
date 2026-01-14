#pragma once

#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <vector>

// wrapper for API GetIpAddrTable and util functions
class IpAddressTable
{
public:
    IpAddressTable();
    ~IpAddressTable();

    IpAddressTable(const IpAddressTable &) = delete;
    IpAddressTable &operator=(const IpAddressTable &) = delete;

    DWORD getAdapterIpAddress(NET_IFINDEX index) const;

private:
    std::vector<unsigned char> arr_;
    MIB_IPADDRTABLE *pIPAddrTable_;
    MIB_UNICASTIPADDRESS_TABLE *pUnicastIPAddrTable_;
};
