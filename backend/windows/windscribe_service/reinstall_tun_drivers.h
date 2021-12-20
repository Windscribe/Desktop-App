#pragma once

class ReinstallTunDrivers
{
public:
	enum class Type { UNKNOWN, TAP, WINTUN };

	static bool reinstallDriver(const Type& type, const wchar_t* driverDir);
};
