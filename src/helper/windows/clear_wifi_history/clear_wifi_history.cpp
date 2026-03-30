#include "clear_wifi_history.h"

#include <windows.h>
#include <winevt.h>
#include <wlanapi.h>

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

#include "utils/systemlibloader.h"
#include "utils/wsscopeguard.h"

bool ClearWiFiHistory::clear()
{
    spdlog::info("Starting WiFi history cleanup");
    
    // Get currently connected WiFi networks to preserve them
    auto connectedProfiles = getCurrentConnectedProfiles();
    if (!connectedProfiles.empty()) {
        spdlog::debug(L"Current connected WiFi network: {}", fmt::join(connectedProfiles, L", "));
    } else {
        spdlog::debug("No currently connected WiFi networks detected");
    }
    
    bool success = true;
    
    // Clear all event logs containing WiFi history
    success &= clearEventLogs();
    
    // Clear all registry entries related to network history
    success &= clearNetworkProfiles();
    success &= clearRegistryConnectionHistory();
    success &= clearManagedNetworkSignatures();
    success &= clearNlaCache();
    
    // Clear WiFi profile files from filesystem (except currently connected)
    success &= clearWlanProfileFiles(connectedProfiles);
    
    if (success) {
        spdlog::info("WiFi history cleanup completed successfully");
    } else {
        spdlog::warn("WiFi history cleanup completed with some errors");
    }
    
    return success;
}

// =============================================================================
// WiFi Detection
// =============================================================================

std::set<std::wstring> ClearWiFiHistory::getCurrentConnectedProfiles()
{
    std::set<std::wstring> connectedProfiles;
    
    try {
        // Load the DLL dynamically as Windows server OSes may not have the Wireless LAN Service installed. The DLL will only
        // exist on the system if said service is installed, and we don't want to block Windscribe install on the server OS
        // by statically linking to the DLL.
        wsl::SystemLibLoader wlanLib("wlanapi.dll");
        const auto wlanOpenHandle = wlanLib.getFunction<DWORD WINAPI(DWORD, PVOID, PDWORD, PHANDLE)>("WlanOpenHandle");
        const auto wlanCloseHandle = wlanLib.getFunction<DWORD WINAPI(HANDLE, PVOID)>("WlanCloseHandle");
        const auto wlanEnumInterfaces = wlanLib.getFunction<DWORD WINAPI(HANDLE, PVOID, PWLAN_INTERFACE_INFO_LIST*)>("WlanEnumInterfaces");
        const auto wlanQueryInterface = wlanLib.getFunction<DWORD WINAPI(HANDLE, const GUID*, WLAN_INTF_OPCODE, PVOID, PDWORD, PVOID*, PWLAN_OPCODE_VALUE_TYPE)>("WlanQueryInterface");
        const auto wlanFreeMemory = wlanLib.getFunction<VOID WINAPI(PVOID)>("WlanFreeMemory");

        HANDLE hClient = NULL;
        DWORD dwClientVersion = 2;
        DWORD dwCurVersion = 0;

        // Initialize WLAN handle
        DWORD dwResult = wlanOpenHandle(dwClientVersion, NULL, &dwCurVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            spdlog::warn("WlanOpenHandle failed with error: {}", dwResult);
            return connectedProfiles;
        }

        auto exitGuard = wsl::wsScopeGuard([&] {
            wlanCloseHandle(hClient, NULL);
        });

        // Enumerate WLAN interfaces
        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        dwResult = wlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS) {
            spdlog::warn("WlanEnumInterfaces failed with error: {}", dwResult);
            return connectedProfiles;
        }

        if (pIfList != NULL) {
            // Iterate through all interfaces
            for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
                PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];

                // Check if interface is connected
                if (pIfInfo->isState == wlan_interface_state_connected) {
                    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
                    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);

                    // Get connection attributes
                    dwResult = wlanQueryInterface(
                        hClient,
                        &pIfInfo->InterfaceGuid,
                        wlan_intf_opcode_current_connection,
                        NULL,
                        &connectInfoSize,
                        (PVOID*)&pConnectInfo,
                        NULL
                    );

                    if (dwResult == ERROR_SUCCESS) {
                        // Get profile name (this is what we need to preserve)
                        std::wstring profileName(pConnectInfo->strProfileName);
                        connectedProfiles.insert(profileName);

                        wlanFreeMemory(pConnectInfo);
                    }
                }
            }
            wlanFreeMemory(pIfList);
        }
    } catch (const std::exception& e) {
        spdlog::warn("getCurrentConnectedProfiles failed: {}", e.what());
    }

    return connectedProfiles;
}

bool ClearWiFiHistory::clearEventLogs()
{
    const wchar_t* eventLogs[] = {
        L"Microsoft-Windows-WLAN-AutoConfig/Operational", // WLAN-AutoConfig operational event log
        L"Microsoft-Windows-NetworkProfile/Operational",  // Network profile change events
        L"Microsoft-Windows-NCSI/Operational"             // Network connectivity detection events
    };

    bool overallSuccess = true;
    
    for (const auto* logPath : eventLogs) {
        BOOL result = EvtClearLog(NULL, logPath, NULL, 0);
        if (result) {
            spdlog::debug(L"Event log cleared successfully: {}", logPath);
        } else {
            DWORD dwError = GetLastError();
            spdlog::warn(L"Failed to clear event log: {} (Error: {})", logPath, dwError);
            overallSuccess = false;
        }
    }
    
    return overallSuccess;
}

bool ClearWiFiHistory::clearNetworkProfiles()
{
    const std::wstring subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles";
    return clearRegistry(subKey);
}

bool ClearWiFiHistory::clearRegistryConnectionHistory()
{
    const std::wstring subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Unmanaged";
    return clearRegistry(subKey);
}

bool ClearWiFiHistory::clearManagedNetworkSignatures()
{
    const std::wstring subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Managed";
    return clearRegistry(subKey);
}

bool ClearWiFiHistory::clearNlaCache()
{
    bool success = true;

    // Try to clear both possible NLA cache locations
    // Different Windows versions may have different structures
    const std::wstring nlaPaths[] = {
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Nla\\Cache",
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Nla\\Wireless"
    };

    for (const auto& subKey : nlaPaths) {
        winreg::RegKey key;
        winreg::RegResult result = key.TryOpen(HKEY_LOCAL_MACHINE, subKey, KEY_READ);

        // Check if key exists before trying to clear it
        if (result.IsOk()) {
            key.Close();
            if (!clearRegistry(subKey)) {
                success = false;
            }
        } else {
            // Key doesn't exist - this is fine, just skip it
            spdlog::debug(L"NLA registry key not found (skipping): {}", subKey);
        }
    }

    return success;
}

// =============================================================================
// File System Cleanup
// =============================================================================

bool ClearWiFiHistory::clearWlanProfileFiles(const std::set<std::wstring>& connectedProfiles)
{
    bool overallSuccess = true;

    try {
        wsl::SystemLibLoader wlanLib("wlanapi.dll");
        const auto wlanOpenHandle = wlanLib.getFunction<DWORD WINAPI(DWORD, PVOID, PDWORD, PHANDLE)>("WlanOpenHandle");
        const auto wlanCloseHandle = wlanLib.getFunction<DWORD WINAPI(HANDLE, PVOID)>("WlanCloseHandle");
        const auto wlanEnumInterfaces = wlanLib.getFunction<DWORD WINAPI(HANDLE, PVOID, PWLAN_INTERFACE_INFO_LIST*)>("WlanEnumInterfaces");
        const auto wlanGetProfileList = wlanLib.getFunction<DWORD WINAPI(HANDLE, const GUID*, PVOID, PWLAN_PROFILE_INFO_LIST*)>("WlanGetProfileList");
        const auto wlanDeleteProfile = wlanLib.getFunction<DWORD WINAPI(HANDLE, const GUID*, LPCWSTR, PVOID)>("WlanDeleteProfile");
        const auto wlanFreeMemory = wlanLib.getFunction<VOID WINAPI(PVOID)>("WlanFreeMemory");

        HANDLE hClient = NULL;
        DWORD dwMaxClient = 2;
        DWORD dwClientVersion = 0;

        // Initialize WLAN handle
        DWORD dwResult = wlanOpenHandle(dwMaxClient, NULL, &dwClientVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            spdlog::warn("WlanOpenHandle failed with error: {}", dwResult);
            return false;
        }

        auto exitGuard = wsl::wsScopeGuard([&] {
            wlanCloseHandle(hClient, NULL);
        });

        int totalDeleted = 0;
        int totalPreserved = 0;

        // Enumerate WLAN interfaces
        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        dwResult = wlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS) {
            spdlog::warn("WlanEnumInterfaces failed with error: {}", dwResult);
            return false;
        }

        if (pIfList != NULL) {
            // Iterate through all interfaces
            for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
                PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];

                // Get list of profiles for this interface
                PWLAN_PROFILE_INFO_LIST pProfileList = NULL;
                dwResult = wlanGetProfileList(hClient, &pIfInfo->InterfaceGuid, NULL, &pProfileList);

                if (dwResult != ERROR_SUCCESS) {
                    spdlog::warn("WlanGetProfileList failed for interface {} with error: {}", i, dwResult);
                    overallSuccess = false;
                    continue;
                }
                if (pProfileList == nullptr) {
                    spdlog::error("pProfileList = nullptr in ClearWiFiHistory::clearWlanProfileFiles");
                    overallSuccess = false;
                    continue;
                }

                // Iterate through profiles
                for (DWORD j = 0; j < pProfileList->dwNumberOfItems; j++) {
                    PWLAN_PROFILE_INFO pProfile = &pProfileList->ProfileInfo[j];
                    std::wstring profileName(pProfile->strProfileName);

                    // Check if this profile is currently connected
                    if (connectedProfiles.find(profileName) != connectedProfiles.end()) {
                        spdlog::debug(L"Preserving connected WiFi profile: {}", profileName);
                        totalPreserved++;
                        continue;
                    }

                    // Delete the profile
                    dwResult = wlanDeleteProfile(hClient, &pIfInfo->InterfaceGuid, profileName.c_str(), NULL);

                    if (dwResult == ERROR_SUCCESS) {
                        spdlog::debug(L"Deleted WiFi profile: {}", profileName);
                        totalDeleted++;
                    } else {
                        spdlog::warn(L"Failed to delete WiFi profile '{}': error {}", profileName, dwResult);
                        overallSuccess = false;
                    }
                }

                if (pProfileList != NULL) {
                    wlanFreeMemory(pProfileList);
                }
            }

            wlanFreeMemory(pIfList);
        }

        spdlog::debug("WiFi profile cleanup: {} profiles deleted, {} preserved", totalDeleted, totalPreserved);
    } catch (const std::exception& e) {
        spdlog::warn("clearWlanProfileFiles failed: {}", e.what());
        return false;
    }

    return overallSuccess;
}

// =============================================================================
// Helper Methods
// =============================================================================

bool ClearWiFiHistory::clearRegistry(const std::wstring &subKey)
{
    winreg::RegKey key;

    // Open the registry key with permissions to read structure and delete subkeys
    winreg::RegResult result = key.TryOpen(HKEY_LOCAL_MACHINE, subKey, KEY_READ | DELETE);
    if (result.Failed()) {
        spdlog::warn(L"Failed to open registry key '{}': {}", subKey, result.ErrorMessage());
        return false;
    }

    // Enumerate all subkeys under this key
    auto keysResult = key.TryEnumSubKeys();
    if (!keysResult.IsValid()) {
        spdlog::warn(L"Failed to enumerate subkeys in '{}': {}", subKey, keysResult.GetError().ErrorMessage());
        return false;
    }

    // Get the list of subkey names
    const std::vector<std::wstring>& subkeyNames = keysResult.GetValue();

    // Delete each subkey recursively
    int deletedCount = 0;
    for (const auto& subkeyName : subkeyNames) {
        result = key.TryDeleteTree(subkeyName);
        if (result.Failed()) {
            spdlog::warn(L"Failed to delete registry subkey '{}\\{}': {}", subKey, subkeyName, result.ErrorMessage());
        } else {
            deletedCount++;
        }
    }

    spdlog::debug(L"Registry cleanup: {} of {} subkeys deleted from '{}'",
                 deletedCount, subkeyNames.size(), subKey);

    return deletedCount ==  subkeyNames.size();
}
