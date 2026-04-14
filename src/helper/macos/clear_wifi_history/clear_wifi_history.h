#pragma once
#include <string>
#include <vector>

// ClearWiFiHistory - Comprehensive WiFi history cleanup utility for macOS
//
// PURPOSE:
// This class removes all traces of WiFi network history to protect user privacy and location data.
// WiFi network data (SSIDs, MAC addresses, connection timestamps) can be correlated with public
// databases like wigle.net to track user movement history and physical locations over time.
//
// WHAT IS CLEARED:
// 1. WiFi Known Networks - Preferred wireless networks
// 2. Network Preferences - SystemConfiguration plist files
// 3. WiFi Keychain Entries - Stored WiFi passwords
// 4. SystemConfiguration Cache - Cached network information
// 5. Wireless Diagnostics Logs - WiFi connection history logs
// 6. Network Location Cache - Location-based network data

// All used external tools are fully supported on macOS 13+ target of this project.

class ClearWiFiHistory
{
public:
    static bool clear();

private:
    static std::string getCurrentConnectedSSID();
    static bool removeAllPreferredNetworks(const std::string &currentSSID);
    static std::vector<std::string> getWiFiInterfaces();
    static bool clearWiFiPasswordsFromKeychain(const std::string &currentSSID);
    static bool clearWiFiDiagnosticLogs();
};
