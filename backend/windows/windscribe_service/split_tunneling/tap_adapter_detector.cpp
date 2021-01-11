#include "../all_headers.h"

#include "tap_adapter_detector.h"
#include "../adapters_info.h"
#include "ip_address_table.h"
#include "../logger.h"

bool TapAdapterDetector::detect(DWORD &outIp, NET_LUID &outLuid)
{
	PMIB_IPINTERFACE_TABLE pipTable = NULL;
	DWORD dwRetVal = GetIpInterfaceTable(AF_INET, &pipTable);
	if (dwRetVal != NO_ERROR)
	{
		return false;
	}

	AdaptersInfo adaptersInfo;
	IpAddressTable addrTable;

	for (ULONG i = 0; i < pipTable->NumEntries; i++)
	{
		if (adaptersInfo.isWindscribeAdapter(pipTable->Table[i].InterfaceIndex))
		{
			if (pipTable->Table[i].Connected)
			{
				outIp = addrTable.getAdapterIpAddress(pipTable->Table[i].InterfaceIndex);
				outLuid = pipTable->Table[i].InterfaceLuid;
				return true;
			}
		}
	}

	return false;
}
