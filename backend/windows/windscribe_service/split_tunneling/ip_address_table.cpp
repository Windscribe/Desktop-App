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


	
}


DWORD IpAddressTable::getAdapterIpAddress(NET_IFINDEX index)
{
	for (DWORD i = 0; i < pIPAddrTable_->dwNumEntries; i++)
	{
		if (pIPAddrTable_->table[i].dwIndex == index)
		{
			return pIPAddrTable_->table[i].dwAddr;
		}
	}
	return 0;
}