#pragma once

class CloseTcpConnections
{
public:
    static void closeAllTcpConnections(bool keepLocalSockets);
private:
    static bool isWindscribeProcessName(DWORD dwPid);
    static bool isLocalAddress(DWORD address);
};

