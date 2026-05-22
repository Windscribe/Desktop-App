#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <Security/Security.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sstream>
#include "clear_wifi_history.h"
#include "../utils.h"

bool ClearWiFiHistory::clear()
{
    spdlog::info("Starting WiFi history cleanup for macOS");
    
    bool success = true;
    
    // Get current connected network before clearing
    std::string currentSSID = getCurrentConnectedSSID();
    if (!currentSSID.empty()) {
        spdlog::debug("Current connected WiFi network: {}", currentSSID);
    } else {
        spdlog::debug("No WiFi network currently connected");
    }
    
    // Remove all preferred networks (except current)
    // Note: This does NOT disconnect from current network
    success &= removeAllPreferredNetworks(currentSSID);

    // Clear password from keychain (except current)
    success &= clearWiFiPasswordsFromKeychain(currentSSID);
    
    // Clear logs and diagnostics
    success &= clearWiFiDiagnosticLogs();
    
    if (success) {
        spdlog::info("WiFi history cleanup completed successfully");
    } else {
        spdlog::warn("WiFi history cleanup completed with some failures");
    }
    
    return success;
}

std::string ClearWiFiHistory::getCurrentConnectedSSID()
{
    std::string currentSSID;

    auto ifaces = getWiFiInterfaces();
    if (ifaces.empty()) {
        spdlog::warn("No WiFi interface found");
        return currentSSID;
    }
    const std::string &wifiInterface = ifaces.front();

    // Get SSID using ipconfig getsummary
    std::string ipconfigOutput;
    std::vector<std::string> args = {"getsummary", wifiInterface};
    int ipconfigResult = Utils::executeCommand("ipconfig", args, &ipconfigOutput);
    
    if (ipconfigResult == 0 && !ipconfigOutput.empty()) {
        // Look for "SSID : NetworkName" in the output
        size_t ssidPos = ipconfigOutput.find(" SSID : ");
        if (ssidPos != std::string::npos) {
            ssidPos += 8; // skip " SSID : "
            size_t endPos = ipconfigOutput.find("\n", ssidPos);
            if (endPos != std::string::npos) {
                currentSSID = ipconfigOutput.substr(ssidPos, endPos - ssidPos);
                // Trim whitespace
                currentSSID.erase(currentSSID.find_last_not_of(" \n\r\t") + 1);
            }
        }
    }
    
    return currentSSID;
}

bool ClearWiFiHistory::removeAllPreferredNetworks(const std::string &currentSSID)
{
    spdlog::debug("Removing all preferred wireless networks except current");
    
    auto interfaces = getWiFiInterfaces();
    bool success = true;
    
    if (interfaces.empty()) {
        spdlog::warn("No WiFi interfaces found");
        return true; // Not a failure - just no WiFi hardware
    }
    
    for (const auto &iface : interfaces) {
        spdlog::debug("Clearing networks for interface: {}", iface);
        
        // Get list of all preferred networks
        std::string networksOutput;
        int listResult = Utils::executeCommand("networksetup",
            {"-listpreferredwirelessnetworks", iface}, &networksOutput);
        
        if (listResult != 0) {
            spdlog::warn("Failed to list preferred networks for interface: {}", iface);
            continue;
        }
        
        // Parse network list and remove each network except current
        // Format: "Preferred networks on en0:\n\tNetworkName1\n\tNetworkName2"
        std::istringstream stream(networksOutput);
        std::string line;
        int removedCount = 0;
        int skippedCount = 0;
        
        while (std::getline(stream, line)) {
            // Skip header line
            if (line.find("Preferred networks on") != std::string::npos) {
                continue;
            }
            
            // Network names are prefixed with tab
            if (line.empty() || line[0] != '\t') {
                continue;
            }
            
            // Extract network name (remove leading tab)
            std::string networkName = line.substr(1);
            // Trim trailing whitespace
            networkName.erase(networkName.find_last_not_of(" \n\r\t") + 1);
            
            if (networkName.empty()) {
                continue;
            }
            
            // Skip if this is the current connected network
            if (!currentSSID.empty() && networkName == currentSSID) {
                spdlog::debug("Skipping current connected network: {}", networkName);
                skippedCount++;
                continue;
            }
            
            // Remove this network
            spdlog::info("Removing network: {}", networkName);
            int removeResult = Utils::executeCommand("networksetup",
                {"-removepreferredwirelessnetwork", iface, networkName});
            
            if (removeResult == 0) {
                removedCount++;
                spdlog::debug("Successfully removed network: {}", networkName);
            } else {
                spdlog::warn("Failed to remove network: {}", networkName);
                success = false;
            }
        }
        
        spdlog::debug("Removed {} network(s), kept {} network(s) for interface: {}", 
                     removedCount, skippedCount, iface);
    }
    
    return success;
}

std::vector<std::string> ClearWiFiHistory::getWiFiInterfaces()
{
    std::vector<std::string> interfaces;
    std::string output;
    
    // Get list of network services
    int result = Utils::executeCommand("networksetup", {"-listallhardwareports"}, &output);
    
    if (result != 0) {
        spdlog::warn("Failed to list hardware ports");
        // Fallback: try common WiFi interfaces
        interfaces.push_back("en0");
        return interfaces;
    }
    
    // Parse output to find WiFi interfaces
    // Format: "Hardware Port: Wi-Fi\nDevice: en0"
    size_t pos = 0;
    while ((pos = output.find("Hardware Port: Wi-Fi", pos)) != std::string::npos) {
        size_t devicePos = output.find("Device: ", pos);
        if (devicePos != std::string::npos) {
            devicePos += 8; // strlen("Device: ")
            size_t endPos = output.find("\n", devicePos);
            if (endPos != std::string::npos) {
                std::string iface = output.substr(devicePos, endPos - devicePos);
                // Trim whitespace
                iface.erase(iface.find_last_not_of(" \n\r\t") + 1);
                if (!iface.empty()) {
                    interfaces.push_back(iface);
                    spdlog::debug("Found WiFi interface: {}", iface);
                }
            }
        }
        pos++;
    }
    
    // Fallback: if no interfaces found, try common ones
    if (interfaces.empty()) {
        spdlog::debug("No WiFi interfaces found in networksetup output, trying en0");
        interfaces.push_back("en0");
    }
    
    return interfaces;
}

bool ClearWiFiHistory::clearWiFiPasswordsFromKeychain(const std::string &currentSSID)
{
    spdlog::debug("Clearing WiFi passwords from keychain except current");

    // Enumerate generic-password keychain items with description "AirPort network password"
    // — that's the canonical macOS storage label for stored Wi-Fi passwords. The Account
    // attribute holds the SSID. Dedup via NSSet since the same SSID can appear multiple times.
    NSDictionary *query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrDescription: @"AirPort network password",
        (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitAll,
        (__bridge id)kSecReturnAttributes: @YES,
    };

    CFTypeRef result = NULL;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);
    if (status == errSecItemNotFound) {
        spdlog::debug("No WiFi passwords found in keychain");
        return true;
    }
    if (status != errSecSuccess) {
        spdlog::warn("SecItemCopyMatching failed: {}", (int)status);
        return false;
    }

    NSArray *items = (__bridge_transfer NSArray *)result;
    NSMutableSet *ssids = [NSMutableSet set];
    for (NSDictionary *item in items) {
        NSString *acct = item[(__bridge id)kSecAttrAccount];
        if ([acct isKindOfClass:[NSString class]] && acct.length > 0) {
            [ssids addObject:acct];
        }
    }

    int deletedCount = 0;
    int skippedCount = 0;
    for (NSString *nsSsid in ssids) {
        std::string ssid([nsSsid UTF8String]);

        if (!currentSSID.empty() && ssid == currentSSID) {
            spdlog::debug("Skipping password for current network: {}", ssid);
            skippedCount++;
            continue;
        }

        int rc = Utils::executeCommand("security",
            {"delete-generic-password",
             "-D", "AirPort network password",
             "-a", ssid},
            nullptr);

        if (rc == 0) {
            deletedCount++;
            spdlog::debug("Successfully deleted password for: {}", ssid);
        } else {
            spdlog::warn("Failed to delete password for: {}", ssid);
        }
    }

    spdlog::debug("Deleted {} WiFi password(s), kept {} password(s)", deletedCount, skippedCount);
    return true;
}

bool ClearWiFiHistory::clearWiFiDiagnosticLogs()
{
    spdlog::debug("Clearing WiFi diagnostic logs");

    struct LogPath {
        std::filesystem::path dir;
        std::string prefix;
    };
    const std::vector<LogPath> logPaths = {
        {"/var/log", "wifi.log"},
        {"/Library/Logs/DiagnosticReports", "WiFi"},
        {"/private/var/log", "wifi-"},
    };

    for (const auto &lp : logPaths) {
        std::error_code ec;
        if (!std::filesystem::exists(lp.dir, ec)) {
            continue;
        }
        for (std::filesystem::directory_iterator it(lp.dir, ec), end; !ec && it != end; it.increment(ec)) {
            const std::string name = it->path().filename().string();
            if (name.rfind(lp.prefix, 0) != 0) {
                continue;
            }
            std::error_code rmEc;
            std::filesystem::remove_all(it->path(), rmEc);
            if (rmEc) {
                spdlog::debug("Failed to remove {}: {}", it->path().string(), rmEc.message());
            } else {
                spdlog::debug("Removed {}", it->path().string());
            }
        }
    }

    return true;
}
