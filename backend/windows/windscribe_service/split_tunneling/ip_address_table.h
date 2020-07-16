#pragma once

// wrapper for API GetIpAddrTable and util functions
class IpAddressTable
{
public:
	IpAddressTable();

	DWORD getAdapterIpAddress(NET_IFINDEX index);

private:
	std::vector<unsigned char> arr_;
	MIB_IPADDRTABLE *pIPAddrTable_;
};

