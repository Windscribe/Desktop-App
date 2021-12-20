#pragma once

class ClearDnsOnTap
{
public:
	static void clearDns();

private:
	static bool regClearDNS(const char *lpszAdapterName);
};

