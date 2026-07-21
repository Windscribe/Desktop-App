#pragma once

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

// Wrapper for GetIpForwardTable2. Dual-stack (AF_UNSPEC): table holds both IPv4 and IPv6 rows.
// Owns the buffer allocated by Windows and frees it via FreeMibTable on destruction.
class IpForwardTable
{
public:
    IpForwardTable();
    ~IpForwardTable();

    IpForwardTable(const IpForwardTable &) = delete;
    IpForwardTable &operator=(const IpForwardTable &) = delete;

    ULONG count() const;
    const MIB_IPFORWARD_ROW2 *getByIndex(ULONG ind) const;

private:
    PMIB_IPFORWARD_TABLE2 pIpForwardTable_;
};
