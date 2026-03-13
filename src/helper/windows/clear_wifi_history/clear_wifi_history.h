#pragma once

#include <string>
#include <set>

// ClearWiFiHistory - Comprehensive WiFi history cleanup utility for Windows
//
// PURPOSE:
// This class removes all traces of WiFi network history to protect user privacy and location data.
// WiFi network data (SSIDs, MAC addresses, connection timestamps) can be correlated with public
// databases like wigle.net to track user movement history and physical locations over time.
//
// WHAT IS CLEARED:
// 1. Windows Event Logs - WLAN AutoConfig operational logs showing connection events
// 2. Network Profiles - Registry entries for all connected networks (WiFi and others)
// 3. Network Signatures - Registry cache of unmanaged/managed network identifiers
// 4. WLAN Profile Files - XML configuration files for saved WiFi networks (except currently connected)
// 5. NLA Cache - Network Location Awareness cache with network metadata
// 6. Additional Event Logs - NetworkProfile and NCSI operational logs
//
// WHAT IS PRESERVED:
// - Currently connected WiFi network profile and password

class ClearWiFiHistory
{
public:
    static bool clear();

private:
    static std::set<std::wstring> getCurrentConnectedProfiles();
    static bool clearEventLogs();
    static bool clearNetworkProfiles();
    static bool clearRegistryConnectionHistory();
    static bool clearManagedNetworkSignatures();
    static bool clearNlaCache();
    static bool clearWlanProfileFiles(const std::set<std::wstring>& connectedProfiles);
    
    // Generic helper to delete all subkeys under a given registry key
    // Parameters:
    //   subKey - Full registry path under HKEY_LOCAL_MACHINE
    // Returns: true if operation succeeded, false otherwise
    static bool clearRegistry(const std::wstring &subKey);
};
