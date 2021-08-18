#include "../all_headers.h"
#include "ip_address_table.h"

IpAddressTable::IpAddressTable()
{
	DWORD dwSize = 16000;
	arr_.resize(dwSize);
	DWORD ret;

	ret = GetIpAddrTable((PMIB_IPADDRTABLE)&arr_[0], &dwSize, 0);
	if (ret == ERROR_INSUFFICIENT_BUFFER)
	{
		arr_.resize(dwSize);
		ret = GetIpAddrTable((PMIB_IPADDRTABLE)&arr_[0], &dwSize, 0);
	}

	if (ret == NO_ERROR)
	{
		pIPAddrTable_ = (PMIB_IPADDRTABLE)&arr_[0];
	}
	else
	{
		pIPAddrTable_ = NULL;
	}

    if (GetUnicastIpAddressTable(AF_INET, &pUnicastIPAddrTable_) != NO_ERROR)
        pUnicastIPAddrTable_ = nullptr;
}

IpAddressTable::~IpAddressTable()
{
    if (pUnicastIPAddrTable_)
        FreeMibTable(pUnicastIPAddrTable_);
}

DWORD IpAddressTable::getAdapterIpAddress(NET_IFINDEX index) const
{
    // Search in a global IP address table.
    for (DWORD i = 0; i < pIPAddrTable_->dwNumEntries; ++i) {
        if (pIPAddrTable_->table[i].dwIndex == index)
            return pIPAddrTable_->table[i].dwAddr;
    }
    if (pUnicastIPAddrTable_) {
        // Search in a unicast IP address table.
        for (ULONG i = 0; i < pUnicastIPAddrTable_->NumEntries; ++i) {
            if (pUnicastIPAddrTable_->Table[i].InterfaceIndex == index)
                return pUnicastIPAddrTable_->Table[i].Address.Ipv4.sin_addr.S_un.S_addr;
        }
    }
    return 0;
}
