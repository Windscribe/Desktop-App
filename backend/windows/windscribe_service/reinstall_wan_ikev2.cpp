#include "all_headers.h"
#include "reinstall_wan_ikev2.h"
#include <cfgmgr32.h>
#include <newdev.h>
#include "logger.h"

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Newdev.lib")

bool ReinstallWanIkev2::enableDevice()
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
        bool bIkev2DeviceFound = false;
        bool bWanIpDeviceFound = false;
        bool bIkev2DeviceEnabled = false;
        bool bWanIpDeviceEnabled = false;

		DWORD deviceIndex = 0;
		SP_DEVINFO_DATA deviceInfoData;
		ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		DEVPROPTYPE ulPropertyType;
		WCHAR szBuffer[4096];
		DWORD dwSize;

		while (SetupDiEnumDeviceInfo(hDevInfo, deviceIndex, &deviceInfoData))
		{
			deviceIndex++;

			if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC, &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize))
			{
				_wcslwr(szBuffer);  //to lower case
				if (wcsstr(szBuffer, L"wan miniport") != 0 && wcsstr(szBuffer, L"(ikev2)") != 0)
				{
					bIkev2DeviceFound = true;
					if (setEnabledState(&hDevInfo, &deviceInfoData))
					{
						bIkev2DeviceEnabled = true;
					}
				}
				else if (wcsstr(szBuffer, L"wan miniport") != 0 && wcsstr(szBuffer, L"(ip)") != 0)
				{
					bWanIpDeviceFound = true;
					if (setEnabledState(&hDevInfo, &deviceInfoData))
					{
						bWanIpDeviceEnabled = true;
					}
				}
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);

		if (bIkev2DeviceFound)
		{
			Logger::instance().out(L"ReinstallWanIkev2::enableDevice, WAN Miniport (IKEv2) found");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::enableDevice, WAN Miniport (IKEv2) not found");
		}
		if (bIkev2DeviceEnabled)
		{
			Logger::instance().out(L"ReinstallWanIkev2::enableDevice, WAN Miniport (IKEv2) enabled");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::enableDevice, WAN Miniport (IKEv2) failed enable");
		}

		if (bWanIpDeviceFound)
		{
			Logger::instance().out(L"ReinstallWanIkev2::enableDevice, WAN Miniport (IP) found");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) not found");
		}
		if (bWanIpDeviceEnabled)
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) enabled");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) failed enable");
		}


		return bIkev2DeviceFound && bIkev2DeviceEnabled && bWanIpDeviceFound && bWanIpDeviceEnabled;
	}
	else
	{
		return false;
	}
}

bool ReinstallWanIkev2::uninstallDevice()
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
        bool bIkev2DeviceFound = false;
        bool bWanIpDeviceFound = false;
        bool bIkev2DeviceUninstalled = false;
        bool bWanIpDeviceUninstalled = false;

		DWORD deviceIndex = 0;
		SP_DEVINFO_DATA deviceInfoData;
		ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
		deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		DEVPROPTYPE ulPropertyType;
		WCHAR szBuffer[4096];
		DWORD dwSize;

		while (SetupDiEnumDeviceInfo(hDevInfo, deviceIndex, &deviceInfoData))
		{
			deviceIndex++;

			if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC, &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize))
			{
				_wcslwr(szBuffer);  //to lower case
				if (wcsstr(szBuffer, L"wan miniport") != 0 && wcsstr(szBuffer, L"(ikev2)") != 0)
				{
					bIkev2DeviceFound = true;
					if (DiUninstallDevice(NULL, hDevInfo, &deviceInfoData, 0, NULL))
					{
						bIkev2DeviceUninstalled = true;
					}
				}
				else if (wcsstr(szBuffer, L"wan miniport") != 0 && wcsstr(szBuffer, L"(ip)") != 0)
				{
					bWanIpDeviceFound = true;
					if (DiUninstallDevice(NULL, hDevInfo, &deviceInfoData, 0, NULL))
					{
						bWanIpDeviceUninstalled = true;
					}
				}
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);

		if (bIkev2DeviceFound)
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IKEv2) found");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IKEv2) not found");
		}
		if (bIkev2DeviceUninstalled)
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IKEv2) uninstalled");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IKEv2) failed uninstall");
		}

		if (bWanIpDeviceFound)
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) found");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) not found");
		}
		if (bWanIpDeviceUninstalled)
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) uninstalled");
		}
		else
		{
			Logger::instance().out(L"ReinstallWanIkev2::uninstallDevice, WAN Miniport (IP) failed uninstall");
		}


		return bIkev2DeviceFound && bIkev2DeviceUninstalled && bWanIpDeviceFound && bWanIpDeviceUninstalled;
	}
	else
	{
		return false;
	}
}

bool ReinstallWanIkev2::scanForHardwareChanges()
{
	DEVINST     devInst;
	CONFIGRET   status;

	status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);

	if (status != CR_SUCCESS) 
	{
		Logger::instance().out(L"CM_Locate_DevNode failed: %x", status);
		return false;
	}

	status = CM_Reenumerate_DevNode(devInst, 0);
	if (status != CR_SUCCESS) 
	{
		Logger::instance().out(L"CM_Reenumerate_DevNode failed: %x", status);
		return false;
	}

	Logger::instance().out(L"ReinstallWanIkev2::scanForHardwareChanges, succeeded", status);

	return true;
}

bool ReinstallWanIkev2::setEnabledState(HDEVINFO *hDevInfo, SP_DEVINFO_DATA *deviceInfoData)
{
	SP_PROPCHANGE_PARAMS spPropChangeParams;

	spPropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
	spPropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
	spPropChangeParams.Scope = DICS_FLAG_GLOBAL;
	spPropChangeParams.StateChange = DICS_ENABLE;

	// prepare operation
	if (!SetupDiSetClassInstallParams(*hDevInfo, deviceInfoData, (SP_CLASSINSTALL_HEADER*)&spPropChangeParams, sizeof(spPropChangeParams)))
	{
		Logger::instance().out(L"SetupDiSetClassInstallParams failed: %x", GetLastError());
		return false;
	}
	// launch op
	if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, *hDevInfo, deviceInfoData))
	{
		Logger::instance().out(L"SetupDiCallClassInstaller failed: %x", GetLastError());
		return false;
	}
	return true;
}