#include "../all_headers.h"
#include "ip_forward_table.h"


IpForwardTable::IpForwardTable()
{
	ULONG ulSize = sizeof(MIB_IPFORWARDROW) * 32;
	ipForwardVector_.resize(ulSize);
	
	DWORD dwStatus = 0;
	dwStatus = GetIpForwardTable((MIB_IPFORWARDTABLE *)&ipForwardVector_[0], &ulSize, FALSE);

	if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
	{
		ipForwardVector_.resize(ulSize);
		dwStatus = GetIpForwardTable((MIB_IPFORWARDTABLE *)&ipForwardVector_[0], &ulSize, FALSE);
	}

	if (dwStatus != ERROR_SUCCESS)
	{
		ipForwardVector_.clear();
		pIpForwardTable = nullptr;
	}
	else
	{
		pIpForwardTable = (MIB_IPFORWARDTABLE *)&ipForwardVector_[0];
	}
}


DWORD IpForwardTable::count() const
{
	if (pIpForwardTable)
	{
		return pIpForwardTable->dwNumEntries;
	}
	return 0;
}
const MIB_IPFORWARDROW *IpForwardTable::getByIndex(DWORD ind) const
{
	if (pIpForwardTable && ind >= 0 && ind < pIpForwardTable->dwNumEntries)
	{
		return &pIpForwardTable->table[ind];
	}
	return nullptr;
}