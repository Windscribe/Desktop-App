#pragma once

class CloseTcpConnections
{
public:
    static void closeAllTcpConnections(bool keepLocalSockets, bool isExclude = true, const std::vector<std::wstring> &apps = std::vector<std::wstring>());
private:
    static bool isWindscribeProcessName(DWORD dwPid);
	static bool isAppSocket(DWORD dwPid, const std::wstring &app);
	static bool isLocalAddress(DWORD address);
};

