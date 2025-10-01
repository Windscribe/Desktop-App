#pragma once

#include "all_headers.h"

class CloseTcpConnections
{
public:
    static void closeAllTcpConnections(bool keepLocalSockets, bool isExclude = true, const std::vector<std::wstring> &apps = std::vector<std::wstring>());
private:
    static bool isWindscribeProcessName(DWORD dwPid);
    static bool isCtrldProcessName(DWORD dwPid);
    static bool isAppSocket(DWORD dwPid, const std::wstring &app);
    static bool isLocalAddress(DWORD address);
};

