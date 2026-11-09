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
            const DWORD enumErr = ::GetLastError();
            if (enumErr == ERROR_NO_MORE_ITEMS) {
                break;
            }
            spdlog::debug("PhantomWireGuardDevices: SetupDiEnumDeviceInfo failed at index {} ({})", index, enumErr);
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
    const winreg::RegResult openResult = enumKey.TryOpen(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\ROOT\\WireGuard",
                                                         KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | DELETE);
    if (openResult.Failed()) {
        if (openResult.Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"PhantomWireGuardDevices: failed to open ROOT\\WireGuard enum key ({}): {}",
                          openResult.Code(), openResult.ErrorMessage());
        }
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
        // Same short-circuit order as before; the intermediate results are kept only for the failure log.
        const CONFIGRET locateRet = ::CM_Locate_DevNodeW(&devInst, instanceId.data(), CM_LOCATE_DEVNODE_PHANTOM);
        CONFIGRET uninstallRet = CR_SUCCESS;
        bool removed = false;
        if (locateRet == CR_SUCCESS) {
            uninstallRet = ::CM_Uninstall_DevNode(devInst, 0);
            removed = (uninstallRet == CR_SUCCESS);
        }
        LSTATUS deleteTreeRet = ERROR_SUCCESS;
        if (!removed) {
            const winreg::RegResult deleteResult = enumKey.TryDeleteTree(subKey);
            deleteTreeRet = deleteResult.Code();
            removed = !deleteResult.Failed();
        }
        if (removed) {
            spdlog::info(L"PhantomWireGuardDevices: removed phantom device {}", instanceId);
        } else {
            spdlog::error(L"PhantomWireGuardDevices: failed to remove phantom device {} (CM_Locate_DevNodeW {}, CM_Uninstall_DevNode {}, RegDeleteTree {})",
                          instanceId, locateRet, (locateRet == CR_SUCCESS) ? std::to_wstring(uninstallRet) : std::wstring(L"n/a"),
                          deleteTreeRet);
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
