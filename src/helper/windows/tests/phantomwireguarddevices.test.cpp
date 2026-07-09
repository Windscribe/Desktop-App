// Creates fake ROOT\WireGuard device nodes the same way the Windows update driver-store walk does, plus a
// registry-only devnode with no PnP registration (the "debris the PnP manager does not recognize" case),
// then verifies PhantomWireGuardDevices::remove() deletes every one of them. Requires elevation; exits 77
// (skip) when run from a non-elevated shell.

#include <Windows.h>

#include <Cfgmgr32.h>
#include <devguid.h>
#include <SetupAPI.h>

#include <cstdio>
#include <string>
#include <vector>

#include "wireguard/phantom_wireguard_devices.h"

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

bool isElevated()
{
    HANDLE token;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation = {};
    DWORD size = sizeof(elevation);
    BOOL ok = ::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    ::CloseHandle(token);
    return ok && elevation.TokenIsElevated;
}

bool createRootWireGuardDevnode(std::wstring &instanceId)
{
    HDEVINFO devInfo = ::SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET, NULL);
    if (devInfo == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool ok = false;
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (::SetupDiCreateDeviceInfoW(devInfo, L"WireGuard", &GUID_DEVCLASS_NET, NULL, NULL, DICD_GENERATE_ID,
                                   &devInfoData)) {
        // Extra explicit terminator: SPDRP_HARDWAREID is REG_MULTI_SZ and needs a double NUL.
        static const WCHAR kHwids[] = L"WireGuard\0";
        if (::SetupDiSetDeviceRegistryPropertyW(devInfo, &devInfoData, SPDRP_HARDWAREID, (const BYTE *)kHwids,
                                                sizeof(kHwids))) {
            WCHAR id[MAX_DEVICE_ID_LEN];
            if (::SetupDiCallClassInstaller(DIF_REGISTERDEVICE, devInfo, &devInfoData) &&
                ::SetupDiGetDeviceInstanceIdW(devInfo, &devInfoData, id, _countof(id), NULL)) {
                instanceId = id;
                ok = true;
            }
        }
    }

    if (!ok) {
        printf("createRootWireGuardDevnode failed (%lu)\n", ::GetLastError());
    }
    ::SetupDiDestroyDeviceInfoList(devInfo);
    return ok;
}

std::wstring instanceSubkey(const std::wstring &instanceId)
{
    return instanceId.substr(instanceId.rfind(L'\\') + 1);
}

bool devnodePresentInPnp(std::wstring instanceId)
{
    DEVINST devInst;
    return ::CM_Locate_DevNodeW(&devInst, instanceId.data(), CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS;
}

// Failure-path teardown is kept independent of PhantomWireGuardDevices::remove(), since a failing run
// may mean remove() itself regressed.
void removeDevnodeDirect(const std::wstring &instanceId)
{
    HDEVINFO devInfo = ::SetupDiCreateDeviceInfoList(NULL, NULL);
    if (devInfo == INVALID_HANDLE_VALUE) {
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (::SetupDiOpenDeviceInfoW(devInfo, instanceId.c_str(), NULL, 0, &devInfoData)) {
        SP_REMOVEDEVICE_PARAMS params;
        params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        params.ClassInstallHeader.InstallFunction = DIF_REMOVE;
        params.Scope = DI_REMOVEDEVICE_GLOBAL;
        params.HwProfile = 0;
        if (::SetupDiSetClassInstallParamsW(devInfo, &devInfoData, &params.ClassInstallHeader, sizeof(params))) {
            ::SetupDiCallClassInstaller(DIF_REMOVE, devInfo, &devInfoData);
        }
    }
    ::SetupDiDestroyDeviceInfoList(devInfo);
}

int countPnpDevnodes()
{
    HDEVINFO devInfo = ::SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, L"ROOT\\WireGuard", NULL, 0, NULL, NULL, NULL);
    if (devInfo == INVALID_HANDLE_VALUE) {
        return -1;
    }

    int count = 0;
    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!::SetupDiEnumDeviceInfo(devInfo, index, &devInfoData)) {
            break;
        }
        ++count;
    }
    ::SetupDiDestroyDeviceInfoList(devInfo);
    return count;
}

int countRegistryEntries()
{
    HKEY enumKey;
    LSTATUS status = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\ROOT\\WireGuard", 0,
                                     KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &enumKey);
    if (status == ERROR_FILE_NOT_FOUND) {
        return 0;
    }
    if (status != ERROR_SUCCESS) {
        return -1;
    }

    DWORD subKeys = 0;
    if (::RegQueryInfoKeyW(enumKey, NULL, NULL, NULL, &subKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) !=
        ERROR_SUCCESS) {
        subKeys = (DWORD)-1;
    }
    ::RegCloseKey(enumKey);
    return (int)subKeys;
}

std::wstring debrisKeyPath(const wchar_t *instanceName)
{
    return std::wstring(L"SYSTEM\\CurrentControlSet\\Enum\\ROOT\\WireGuard\\") + instanceName;
}

// A bare registry devnode key with no SetupAPI/PnP registration behind it: the debris that only
// removePhantomRegistryEntries()'s RegDeleteTreeW fallback can clean, which is why removing it needs
// DELETE access on the enum key. The PnP-registered devnodes above never exercise that branch.
bool createRegistryOnlyDebris(const wchar_t *instanceName)
{
    HKEY key;
    LSTATUS status = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE, debrisKeyPath(instanceName).c_str(), 0, NULL, 0,
                                       KEY_WRITE, NULL, &key, NULL);
    if (status != ERROR_SUCCESS) {
        printf("createRegistryOnlyDebris failed (%ld)\n", status);
        return false;
    }
    ::RegCloseKey(key);
    return true;
}

bool registryKeyExists(const wchar_t *instanceName)
{
    HKEY key;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, debrisKeyPath(instanceName).c_str(), 0, KEY_READ, &key) !=
        ERROR_SUCCESS) {
        return false;
    }
    ::RegCloseKey(key);
    return true;
}

// PnP completes devnode removal asynchronously after DIF_REMOVE returns, so poll briefly instead of
// asserting the postconditions instantly.
bool waitForRemoval(const std::wstring &instanceId, DWORD timeoutMs)
{
    for (DWORD waited = 0;; waited += 100) {
        if (!devnodePresentInPnp(instanceId) && !registryKeyExists(instanceSubkey(instanceId).c_str())) {
            return true;
        }
        if (waited >= timeoutMs) {
            return false;
        }
        ::Sleep(100);
    }
}

}

int main()
{
    if (!isElevated()) {
        printf("SKIPPED: must run from an elevated shell (creates and removes PnP device nodes)\n");
        return 77;
    }

    const int preexisting = countRegistryEntries();
    printf("Pre-existing ROOT\\WireGuard entries: %d\n", preexisting);
    VERIFY(preexisting >= 0);

    std::vector<std::wstring> createdIds;
    for (int i = 0; i < 3; ++i) {
        std::wstring instanceId;
        if (VERIFY(createRootWireGuardDevnode(instanceId))) {
            createdIds.push_back(instanceId);
        }
    }
    VERIFY(countRegistryEntries() == preexisting + 3);
    VERIFY(countPnpDevnodes() >= 3);

    PhantomWireGuardDevices::remove();

    // Postconditions are scoped to the devnodes this run created: a pre-existing phantom that resists
    // removal must not fail a run whose own fixtures were cleaned correctly.
    for (const std::wstring &id : createdIds) {
        VERIFY(waitForRemoval(id, 5000));
    }

    // Registry-only debris: a devnode key the PnP manager does not back, which only the RegDeleteTreeW
    // fallback can remove. This is the case the PnP-registered nodes above never reach.
    const wchar_t *kDebrisInstance = L"ws_phantom_test_0000";
    VERIFY(createRegistryOnlyDebris(kDebrisInstance));
    VERIFY(registryKeyExists(kDebrisInstance));
    PhantomWireGuardDevices::remove();
    VERIFY(!registryKeyExists(kDebrisInstance));

    if (g_failures > 0) {
        for (const std::wstring &id : createdIds) {
            removeDevnodeDirect(id);
        }
        ::RegDeleteTreeW(HKEY_LOCAL_MACHINE, debrisKeyPath(kDebrisInstance).c_str());
        printf("%d failure(s)\n", g_failures);
        return 1;
    }

    printf("PASS: fake ROOT\\WireGuard devnodes were created and fully removed\n");
    return 0;
}
