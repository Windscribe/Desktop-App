#include "all_headers.h"
#include "ip_address.h"
#include <Mstcpip.h>

IpAddress::IpAddress(PCWSTR address)
{
    UINT16 port = 0;
    IN_ADDR v4Addr = {0};
    pIPv4Address = 0;
    RtlIpv4StringToAddressEx(address, FALSE, &v4Addr, &port);
    CopyMemory(&pIPv4Address, &v4Addr, 4);
}

IpAddress::IpAddress(PCSTR address)
{
	UINT16 port = 0;
	IN_ADDR v4Addr = { 0 };
	pIPv4Address = 0;
	RtlIpv4StringToAddressExA(address, FALSE, &v4Addr, &port);
	CopyMemory(&pIPv4Address, &v4Addr, 4);
}

IpAddress::IpAddress(DWORD dwAddress)
{
	pIPv4Address = dwAddress;
}

UINT32 IpAddress::IPv4NetworkOrder()  const
{
    return pIPv4Address;
}

UINT32 IpAddress::IPv4HostOrder() const
{
	return ntohl(pIPv4Address);
}

std::wstring IpAddress::asString() const
{
	IN_ADDR v4Addr = { 0 };
	v4Addr.S_un.S_addr = pIPv4Address;
	wchar_t str[128];
	ULONG len = 128;
	RtlIpv4AddressToStringExW(&v4Addr, NULL, str, &len);
	return str;
}

