#include <windows.h>
#include <devguid.h>
#include <setupapi.h>
#include "../../../Utils/logger.h"
#include "fixadaptername.h"

#pragma comment(lib, "Setupapi.lib")

// static
bool FixAdapterName::applyFix(LPCWSTR hwid)
{
    HDEVINFO device_info = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, nullptr, nullptr, DIGCF_PRESENT);
    if (device_info == INVALID_HANDLE_VALUE) {
        Log::instance().out("FixAdapterName: SetupDiGetClassDevs failed (%lu)", ::GetLastError());
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
            Log::instance().out(
                "FixAdapterName: SetupDiGetDeviceRegistryProperty(SPDRP_DEVICEDESC) failed (%lu)", ::GetLastError());
            break;
        }
        if (!SetupDiGetDeviceRegistryProperty(device_info, &device_info_data, SPDRP_FRIENDLYNAME,
            &property_reg_data_type, reinterpret_cast<BYTE*>(buffer), sizeof(buffer),
            &datasize)) {
            device_ok = false;
            Log::instance().out(
                "FixAdapterName: SetupDiGetDeviceRegistryProperty(SPDRP_FRIENDLYNAME) failed (%lu)", ::GetLastError());
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
            Log::instance().out(
                "FixAdapterName: SetupDiSetDeviceRegistryProperty(SPDRP_FRIENDLYNAME) failed (%lu)", ::GetLastError());
            break;
        }
        Log::instance().out("FixAdapterName: \"%ls\" name fixed:", hwid);
        Log::instance().out("    Old name: %ls", buffer);
        Log::instance().out("    New name: %ls", device_desc);
        break;
    }

    SetupDiDestroyDeviceInfoList(device_info);
    return device_ok;
}
