#include "all_headers.h"
#include "close_tcp_connections.h"

// static
void CloseTcpConnections::closeAllTcpConnections(bool keepLocalSockets)
{
	PMIB_TCPTABLE2 pTcpTable = NULL;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	int i;

	pTcpTable = (MIB_TCPTABLE2 *)malloc(sizeof(MIB_TCPTABLE2));
	if (pTcpTable == NULL)
	{
		return;
	}

	dwSize = sizeof(MIB_TCPTABLE2);
	// Make an initial call to GetTcpTable to get the necessary size into the dwSize variable
	if ((dwRetVal = GetTcpTable2(pTcpTable, &dwSize, TRUE)) == ERROR_INSUFFICIENT_BUFFER)
	{
		free(pTcpTable);
		pTcpTable = (MIB_TCPTABLE2 *)malloc(dwSize);
		if (pTcpTable == NULL)
		{
			return;
		}
	}

	// Make a second call to GetTcpTable to get the actual data we require
	if ((dwRetVal = GetTcpTable2(pTcpTable, &dwSize, TRUE)) == NO_ERROR)
	{
        for (i = 0; i < (int)pTcpTable->dwNumEntries; i++)
        {
            auto *entry = &pTcpTable->table[i];
            // Do not close listening sockets.
            if (entry->dwState == MIB_TCP_STATE_LISTEN)
                continue;
            // Do not close LAN sockets, if explicitly requested.
            if (keepLocalSockets && isLocalAddress(entry->dwRemoteAddr))
                continue;
            // Do not close Windscribe sockets.
            if (isWindscribeProcessName(entry->dwOwningPid))
                continue;

            entry->dwState = MIB_TCP_STATE_DELETE_TCB;
            SetTcpEntry(reinterpret_cast<MIB_TCPROW *>(entry));
        }
	}

	if (pTcpTable != NULL) 
	{
		free(pTcpTable);
	}
}

// static
bool CloseTcpConnections::isWindscribeProcessName(DWORD dwPid)
{
	bool bRet = false;
	HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (processHandle != NULL)
	{
		TCHAR filename[MAX_PATH];
		if (GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH) != 0)
		{
			_wcslwr(filename);
			if (wcsstr(filename, L"windscribe") != 0)
			{
				bRet = true;
			}
		}
		CloseHandle(processHandle);
	}

	return bRet;
}

namespace
{
    // Helper class to generate a 32-bit IP address from bytes in network order.
    template<int A, int B, int C, int D>
    struct AddressFromBytes {
        constexpr DWORD operator()() const { return ((A & 0xff) << 24) | ((B & 0xff) << 16) |
                                                    ((C & 0xff) << 8) | (D & 0xff); }
    };

    // Helper function to generate a pair of IP addresses in network order, with checking the range
    // validity at compile time.
    template<typename A1, typename A2>
    constexpr auto MakeIpAddressRange(A1 a1, A2 a2) {
        static_assert(a1() <= a2(), "MakeIpAddressRange: min > max");
        return std::make_pair(a1(), a2());
    };
}  // namespace

// static
bool CloseTcpConnections::isLocalAddress(DWORD address)
{
    const std::pair<DWORD,DWORD> kLocalAddressRanges[] = {
        MakeIpAddressRange(AddressFromBytes<127,0,0,0>(), AddressFromBytes<127,0,0,255>()),
        MakeIpAddressRange(AddressFromBytes<10,0,0,0>(), AddressFromBytes<10,255,255,255>()),
        MakeIpAddressRange(AddressFromBytes<172,16,0,0>(), AddressFromBytes<172,31,255,255>()),
        MakeIpAddressRange(AddressFromBytes<192,168,0,0>(), AddressFromBytes<192,168,255,255>()),
        MakeIpAddressRange(AddressFromBytes<224,0,0,0>(), AddressFromBytes<239,255,255,255>()),
    };

    // Addresses are in the host order, convert it to the network order.
    const auto network_address = htonl(address);

    for (const auto &local_address_range : kLocalAddressRanges)
        if (network_address >= local_address_range.first &&
            network_address <= local_address_range.second)
            return true;

    return false;
}
