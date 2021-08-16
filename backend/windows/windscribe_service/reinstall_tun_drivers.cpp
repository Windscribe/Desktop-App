#include "all_headers.h"
#include "reinstall_tun_drivers.h"

#include "logger.h"

#include <windows.h>
#include <devguid.h>
#include <setupapi.h>

namespace {
	const wchar_t* tapinstallFilename = L"tapinstall.exe";

	const wchar_t* tapDriverCatalogFilename = L"tapwindscribe0901.cat";
	const wchar_t* tapDriverFilename        = L"tapwindscribe0901.sys";
	const wchar_t* tapDriverInfoFilename    = L"OemVista.inf";
    const wchar_t* tapDriverFriendlyName    = L"tapwindscribe0901";

	const wchar_t* wintunDriverCatalogFilename = L"windtun420.cat";
	const wchar_t* wintunDriverFilename        = L"windtun420.sys";
	const wchar_t* wintunDriverInfoFilename    = L"windtun420.inf";
    const wchar_t* wintunDriverFriendlyName    = L"windtun420";

    // @todo Code duplication: method is a copy of FixAdapterName::applyFix from installer project.
	bool changeDeviceFriendlyName(LPCWSTR hwid) {
        HDEVINFO device_info = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, nullptr, nullptr, DIGCF_PRESENT);
        if (device_info == INVALID_HANDLE_VALUE) {
            Logger::instance().out("ReinstallTunDrivers: SetupDiGetClassDevs failed");
            return false;
        }

        SP_DEVINFO_DATA device_info_data;
        DWORD datasize, property_reg_data_type;
        TCHAR buffer[4096];
        bool device_ok = false;

        // Find an adapter with the hardware ID of interest.
        for (DWORD i = 0; !device_ok; ++i) {
            device_info_data.cbSize = sizeof(device_info_data);
            if (!SetupDiEnumDeviceInfo(device_info, i, &device_info_data))
                break;
            if (SetupDiGetDeviceRegistryProperty(
                device_info, &device_info_data, SPDRP_HARDWAREID, &property_reg_data_type,
                reinterpret_cast<BYTE*>(buffer), sizeof(buffer), &datasize)) {
                const DWORD size_in_chars = datasize / sizeof(TCHAR);
                const LPCTSTR buffer_end = buffer + ARRAYSIZE(buffer);
                for (LPCTSTR szid = buffer; *szid != TEXT('\0') && szid + size_in_chars <= buffer_end;
                    szid += lstrlen(szid) + 1) {
                    if (!lstrcmpi(szid, hwid)) {
                        device_ok = true;
                        break;
                    }
                }
            }
        }

        while (device_ok) {
            // Get device description and friendly name.
            TCHAR device_desc[4096];
            DWORD device_desc_size = 0;
            if (!SetupDiGetDeviceRegistryProperty(device_info, &device_info_data, SPDRP_DEVICEDESC,
                &property_reg_data_type, reinterpret_cast<BYTE*>(device_desc), sizeof(device_desc),
                &device_desc_size)) {
                device_ok = false;
                Logger::instance().out(
                    "ReinstallTunDrivers: SetupDiGetDeviceRegistryProperty(SPDRP_DEVICEDESC) failed");
                break;
            }
            if (!SetupDiGetDeviceRegistryProperty(device_info, &device_info_data, SPDRP_FRIENDLYNAME,
                &property_reg_data_type, reinterpret_cast<BYTE*>(buffer), sizeof(buffer),
                &datasize)) {
                device_ok = false;
                Logger::instance().out(
                    "ReinstallTunDrivers: SetupDiGetDeviceRegistryProperty(SPDRP_FRIENDLYNAME) failed");
                break;
            }
            // Check if device description and friendly name don't match. If that's the case, change the
            // device friendly name to the device description.
            if (!lstrcmp(buffer, device_desc)) {
                // Names are equal, nothing to fix.
                break;
            }
            if (!SetupDiSetDeviceRegistryProperty(
                device_info, &device_info_data, SPDRP_FRIENDLYNAME,
                reinterpret_cast<const BYTE*>(device_desc), device_desc_size)) {
                device_ok = false;
                Logger::instance().out(
                    "ReinstallTunDrivers: SetupDiSetDeviceRegistryProperty(SPDRP_FRIENDLYNAME) failed");
                break;
            }
            Logger::instance().out("ReinstallTunDrivers: \"%ls\" name fixed:", hwid);
            Logger::instance().out("    Old name: %ls", buffer);
            Logger::instance().out("    New name: %ls", device_desc);
            break;
        }

        SetupDiDestroyDeviceInfoList(device_info);
        return device_ok;
	}

    // @todo Code duplication. Method is essentially the same as Process::InstExec in installer project.
    bool runAndWait(const wchar_t* cmd, const wchar_t* workDir, unsigned long& resultCode) {
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.dwFlags = STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = SW_HIDE;

        auto result = CreateProcess(nullptr, const_cast<wchar_t*>(cmd),
            nullptr, nullptr,
            false, CREATE_DEFAULT_ERROR_MODE, nullptr,
            workDir, &startupInfo, &processInfo);

        if (!result)
        {
            resultCode = 0;
            return result;
        }
        CloseHandle(processInfo.hThread);

        WaitForSingleObject(processInfo.hProcess, INFINITE);
        if (!GetExitCodeProcess(processInfo.hProcess, &resultCode))
        {
            resultCode = 0xFFFFFFFF;  //{ just in case }
        }

        CloseHandle(processInfo.hProcess);

        return result;
    }

}

bool ReinstallTunDrivers::reinstallDriver(const Type& type, const wchar_t* driverDir)
{
	try {
		if (type == Type::TAP) {

            const auto tapInstallExePath = std::wstring(driverDir) + L"/" + tapinstallFilename;
            const auto removeCmd  = tapInstallExePath + L" remove " + tapDriverFriendlyName;
            const auto installCmd = tapInstallExePath + L" install " + tapDriverInfoFilename + L" " + tapDriverFriendlyName;
            unsigned long resultCode{};
            if(!runAndWait(removeCmd.c_str(), driverDir, resultCode)){
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to remove tap driver with resultCode=", resultCode);
                return false;
            }
            if(!runAndWait(installCmd.c_str(), driverDir, resultCode)){
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to install tap driver with resultCode=", resultCode);
                return false;
            }

            if (!changeDeviceFriendlyName(tapDriverFriendlyName)) {
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to fix \"%s\" adapter name", tapDriverFriendlyName);
            }

            return true;
		}
		else if (type == Type::WINTUN) {

            const auto tapInstallExePath = std::wstring(driverDir) + L"/" + tapinstallFilename;
            const auto removeCmd = tapInstallExePath + L" remove " + wintunDriverFriendlyName;
            const auto installCmd = tapInstallExePath + L" install " + wintunDriverInfoFilename + L" " + wintunDriverFriendlyName; 
            unsigned long resultCode{};
            if (!runAndWait(removeCmd.c_str(), driverDir, resultCode)) {
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to remove wintun driver with resultCode=", resultCode);
                return false;
            }
            if (!runAndWait(installCmd.c_str(), driverDir, resultCode)) {
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to install wintun driver with resultCode=", resultCode);
                return false;
            }

            if (!changeDeviceFriendlyName(wintunDriverFriendlyName)) {
                Logger::instance().out(L"ReinstallTunDrivers::reinstallDriver, Failed to fix \"%s\" adapter name", wintunDriverFriendlyName);
            }

            return true;
		}
	} 
	catch (const std::exception& ex) {
		Logger::instance().out("ReinstallTunDrivers::reinstallDriver, exception: %s", ex.what());
		return false;
	}

	return false;
}
