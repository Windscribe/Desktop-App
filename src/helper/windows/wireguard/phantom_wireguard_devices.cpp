#include "phantom_wireguard_devices.h"

#include <Windows.h>

#include <Cfgmgr32.h>
#include <devguid.h>
#include <SetupAPI.h>

#include <string>

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

#include "utils/wsscopeguard.h"

namespace
{

void removeRegisteredDevices()
{
    // Flags 0 (not DIGCF_PRESENT): non-present ghost devnodes must be enumerated too.
    HDEVINFO devInfo = ::SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, L"ROOT\\WireGuard", NULL, 0, NULL, NULL, NULL);
    if (devInfo == INVALID_HANDLE_VALUE) {
        spdlog::error("PhantomWireGuardDevices: SetupDiGetClassDevsExW failed ({})", ::GetLastError());
        return;
    }
    auto devInfoCleanup = wsl::wsScopeGuard([&]() { ::SetupDiDestroyDeviceInfoList(devInfo); });

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!::SetupDiEnumDeviceInfo(devInfo, index, &devInfoData)) {
            if (::GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }

        WCHAR instanceId[MAX_DEVICE_ID_LEN] = L"<unknown>";
        ::SetupDiGetDeviceInstanceIdW(devInfo, &devInfoData, instanceId, _countof(instanceId), NULL);

        spdlog::info(L"PhantomWireGuardDevices: removing device {}", instanceId);

        SP_REMOVEDEVICE_PARAMS params;
        params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        params.ClassInstallHeader.InstallFunction = DIF_REMOVE;
        params.Scope = DI_REMOVEDEVICE_GLOBAL;
        params.HwProfile = 0;
        if (!::SetupDiSetClassInstallParamsW(devInfo, &devInfoData, &params.ClassInstallHeader, sizeof(params)) ||
            !::SetupDiCallClassInstaller(DIF_REMOVE, devInfo, &devInfoData)) {
            spdlog::error(L"PhantomWireGuardDevices: failed to remove device {} ({})", instanceId, ::GetLastError());
            continue;
        }

        SP_DEVINSTALL_PARAMS_W installParams;
        installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        // Defensive check beyond upstream: a bogus ROOT devnode has no started device stack, so DIF_REMOVE
        // should never defer to a reboot. Logged in case that assumption ever breaks.
        if (::SetupDiGetDeviceInstallParamsW(devInfo, &devInfoData, &installParams) &&
            (installParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART))) {
            spdlog::warn(L"PhantomWireGuardDevices: removed device {}, but a reboot is required to complete it",
                         instanceId);
        } else {
            spdlog::info(L"PhantomWireGuardDevices: removed device {}", instanceId);
        }
    }
}

void removePhantomRegistryEntries()
{
    winreg::RegKey enumKey;
    // TryDeleteTree (RegDeleteTreeW) requires DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE on the handle.
    if (enumKey.TryOpen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\ROOT\\WireGuard",
                        KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | DELETE).Failed()) {
        return;
    }

    // TryEnumSubKeys snapshots the names before any deletion: relying on the post-delete index shift to
    // advance the enumeration hangs forever if a "successful" removal leaves the key.
    auto subKeysResult = enumKey.TryEnumSubKeys();
    if (!subKeysResult.IsValid()) {
        spdlog::error(L"PhantomWireGuardDevices: failed to enumerate ROOT\\WireGuard entries: {}",
                      subKeysResult.GetError().ErrorMessage());
        return;
    }

    for (const std::wstring &subKey : subKeysResult.GetValue()) {
        std::wstring instanceId = std::wstring(L"ROOT\\WireGuard\\") + subKey;
        DEVINST devInst;
        // CM_Uninstall_DevNode is the proper removal for a phantom devnode; fall back to deleting the raw
        // registry entry for debris the PnP manager does not recognize at all.
        if ((::CM_Locate_DevNodeW(&devInst, instanceId.data(), CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS &&
             ::CM_Uninstall_DevNode(devInst, 0) == CR_SUCCESS) ||
            !enumKey.TryDeleteTree(subKey).Failed()) {
            spdlog::info(L"PhantomWireGuardDevices: removed phantom device {}", instanceId);
        } else {
            spdlog::error(L"PhantomWireGuardDevices: failed to remove phantom device {}", instanceId);
        }
    }
}

}

namespace PhantomWireGuardDevices
{

void remove()
{
    removeRegisteredDevices();
    removePhantomRegistryEntries();
}

}
