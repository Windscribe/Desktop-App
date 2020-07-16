#pragma once

class ReinstallWanIkev2
{
public:
	static bool enableDevice();
	static bool uninstallDevice();
	static bool scanForHardwareChanges();

private:
	static bool setEnabledState(HDEVINFO *hDevInfo, SP_DEVINFO_DATA *deviceInfoData);
};

