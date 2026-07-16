#include "clear_wifi_history.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "../utils.h"

namespace fs = std::filesystem;

bool ClearWiFiHistory::clear()
{
    spdlog::info("Starting WiFi history cleanup for Linux");
    
    bool success = true;
    
    // Get current connected network before clearing
    std::string currentSSID = getCurrentConnectedSSID();
    if (!currentSSID.empty()) {
        spdlog::debug("Current connected WiFi network: {}", currentSSID);
    } else {
        spdlog::debug("No WiFi network currently connected");
    }
    
    // Clear NetworkManager connections (most common on Linux desktops)
    success &= clearNetworkManagerConnections(currentSSID);
    success &= clearNetworkManagerState();
    
    // Clear wpa_supplicant configuration files
    success &= clearWpaSupplicantConfig(currentSSID);
    
    // Clear iwd profiles (if used)
    success &= clearIwdProfiles(currentSSID);
    
    // Clear systemd journal logs
    success &= clearWiFiJournalLogs();
    
    // Restart NetworkManager to apply changes
    reloadNetworkManager();
    
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
    
    // Try NetworkManager first (most common on desktop Linux)
    std::string nmcliOutput;
    int result = Utils::executeCommand("nmcli", 
        {"-t", "-f", "ACTIVE,SSID", "device", "wifi"}, &nmcliOutput);
    
    if (result == 0 && !nmcliOutput.empty()) {
        // Parse output: "yes:NetworkName" or "no:NetworkName"
        std::istringstream stream(nmcliOutput);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("yes:") == 0) {
                currentSSID = line.substr(4); // Skip "yes:"
                // Trim whitespace
                currentSSID.erase(currentSSID.find_last_not_of(" \n\r\t") + 1);
                spdlog::debug("Current connected WiFi network found with nmcli");
                break;
            }
        }
    }

    // If NetworkManager didn't work, try iw dev (low-level kernel interface)
    // This works on any Linux system with WiFi drivers, regardless of network manager
    if (currentSSID.empty()) {
        // Get list of wireless interfaces
        std::string iwDevOutput;
        result = Utils::executeCommand("iw", {"dev"}, &iwDevOutput);
        
        if (result == 0 && !iwDevOutput.empty()) {
            // Parse to find wireless interfaces
            std::istringstream stream(iwDevOutput);
            std::string line;
            std::vector<std::string> wifiInterfaces;
            
            while (std::getline(stream, line)) {
                // Look for "Interface" lines
                size_t pos = line.find("Interface ");
                if (pos != std::string::npos) {
                    std::string iface = line.substr(pos + 10); // Skip "Interface "
                    // Trim whitespace
                    iface.erase(0, iface.find_first_not_of(" \t\r\n"));
                    iface.erase(iface.find_last_not_of(" \t\r\n") + 1);
                    if (!iface.empty()) {
                        wifiInterfaces.push_back(iface);
                    }
                }
            }
            
            // Try each interface to find connected SSID
            for (const auto &iface : wifiInterfaces) {
                std::string iwLinkOutput;
                result = Utils::executeCommand("iw", {"dev", iface, "link"}, &iwLinkOutput);
                
                if (result == 0 && !iwLinkOutput.empty()) {
                    // Look for "SSID: " in output and extract everything after it
                    // Example: "	SSID: WiFi Network 1"
                    size_t ssidPos = iwLinkOutput.find("SSID: ");
                    if (ssidPos != std::string::npos) {
                        size_t start = ssidPos + 6; // Length of "SSID: "
                        size_t end = iwLinkOutput.find("\n", start);
                        if (end == std::string::npos) {
                            end = iwLinkOutput.length();
                        }
                        
                        currentSSID = iwLinkOutput.substr(start, end - start);
                        // Trim whitespace
                        currentSSID.erase(0, currentSSID.find_first_not_of(" \t\r\n"));
                        currentSSID.erase(currentSSID.find_last_not_of(" \t\r\n") + 1);
                        
                        if (!currentSSID.empty()) {
                            spdlog::debug("Current connected WiFi network found with iw dev on interface: {}", iface);
                            break;
                        }
                    }
                }
            }
        }
    }

    return currentSSID;
}

bool ClearWiFiHistory::clearNetworkManagerConnections(const std::string &currentSSID)
{
    spdlog::info("Clearing NetworkManager WiFi connections");
    
    const std::string connectionsDir = "/etc/NetworkManager/system-connections";
    
    if (!Utils::isFileExists(connectionsDir)) {
        spdlog::debug("NetworkManager connections directory not found: {}", connectionsDir);
        return true; // Not an error - just not using NetworkManager
    }
    
    bool success = true;
    int removedCount = 0;
    int skippedCount = 0;
    
    try {
        for (const auto &entry : fs::directory_iterator(connectionsDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string filePath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            
            // Check if this is a WiFi connection file
            if (!isWirelessConfigFile(filePath)) {
                spdlog::debug("Skipping non-wireless file: {}", fileName);
                continue;
            }
            
            // Read the file to find the SSID
            std::ifstream file(filePath);
            if (!file.is_open()) {
                spdlog::warn("Could not open connection file: {}", filePath);
                continue;
            }
            
            std::string ssid;
            std::string line;
            bool inWifiSection = false;
            
            while (std::getline(file, line)) {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
                if (line == "[wifi]") {
                    inWifiSection = true;
                } else if (line.empty() || line[0] == '[') {
                    inWifiSection = false;
                }
                
                if (inWifiSection && line.find("ssid=") == 0) {
                    ssid = line.substr(5);
                    break;
                }
            }
            file.close();
            
            // Skip if this is the currently connected network
            if (!currentSSID.empty() && ssid == currentSSID) {
                spdlog::debug("Skipping current connected network: {}", ssid);
                skippedCount++;
                continue;
            }
            
            // Delete the connection file
            spdlog::debug("Removing WiFi connection: {} ({})", ssid.empty() ? fileName : ssid, fileName);
            if (deleteFileSafely(filePath)) {
                removedCount++;
            } else {
                success = false;
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("Error while clearing NetworkManager connections: {}", e.what());
        success = false;
    }
    
    spdlog::debug("Removed {} WiFi connection(s), kept {} connection(s)",
                 removedCount, skippedCount);
    
    return success;
}

bool ClearWiFiHistory::clearNetworkManagerState()
{
    spdlog::debug("Clearing NetworkManager state files");
    
    bool success = true;
    
    std::vector<std::string> stateFiles = {
        "/var/lib/NetworkManager/timestamps",
        "/var/lib/NetworkManager/seen-bssids",
        "/var/lib/NetworkManager/NetworkManager-intern.conf"
    };
    
    for (const auto &filePath : stateFiles) {
        if (Utils::isFileExists(filePath)) {
            spdlog::debug("Removing state file: {}", filePath);
            if (!deleteFileSafely(filePath)) {
                success = false;
            }
        } else {
            spdlog::debug("State file not found (skipping): {}", filePath);
        }
    }
    
    return success;
}

// Clears wpa_supplicant configuration files
// Finds and processes ALL .conf files in /etc/wpa_supplicant/ directory
// instead of hardcoded list, supporting any interface naming convention
bool ClearWiFiHistory::clearWpaSupplicantConfig(const std::string &currentSSID)
{
    spdlog::debug("Clearing wpa_supplicant configuration files");
    
    bool success = true;
    const std::string wpaSuppDir = "/etc/wpa_supplicant";
    
    if (!Utils::isFileExists(wpaSuppDir)) {
        spdlog::debug("wpa_supplicant directory not found: {}", wpaSuppDir);
        return true; // Not an error - just not using wpa_supplicant
    }
    
    int processedCount = 0;
    int removedCount = 0;
    int updatedCount = 0;
    
    try {
        // Iterate through all files in wpa_supplicant directory
        for (const auto &entry : fs::directory_iterator(wpaSuppDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string configPath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            
            // Only process .conf files
            if (fileName.find(".conf") != fileName.length() - 5) {
                spdlog::debug("Skipping non-conf file: {}", fileName);
                continue;
            }
            
            // Verify this is actually a wpa_supplicant config by content
            if (!isWpaSupplicantConfigFile(configPath)) {
                spdlog::debug("Skipping non-wpa_supplicant file: {}", fileName);
                continue;
            }
            
            spdlog::debug("Processing wpa_supplicant config: {}", fileName);
            processedCount++;
            
            if (currentSSID.empty()) {
                // No current connection - safe to delete the whole file
                spdlog::debug("Removing wpa_supplicant config: {}", fileName);
                if (deleteFileSafely(configPath)) {
                    removedCount++;
                } else {
                    success = false;
                }
            } else {
                // Need to preserve current network - read, filter, and rewrite
                std::ifstream inFile(configPath);
                if (!inFile.is_open()) {
                    spdlog::warn("Could not open wpa_supplicant config: {}", configPath);
                    success = false;
                    continue;
                }
                
                std::vector<std::string> lines;
                std::string line;
                bool inNetworkBlock = false;
                bool isCurrentNetwork = false;
                std::vector<std::string> currentBlock;
                int removedNetworks = 0;
                int preservedNetworks = 0;
                
                while (std::getline(inFile, line)) {
                    if (line.find("network={") != std::string::npos) {
                        inNetworkBlock = true;
                        isCurrentNetwork = false;
                        currentBlock.clear();
                        currentBlock.push_back(line);
                    } else if (inNetworkBlock) {
                        currentBlock.push_back(line);
                        
                        // Check if this is the current network
                        if (line.find("ssid=\"" + currentSSID + "\"") != std::string::npos) {
                            isCurrentNetwork = true;
                        }
                        
                        if (line.find("}") != std::string::npos) {
                            // End of network block
                            if (isCurrentNetwork) {
                                // Keep this network block
                                lines.insert(lines.end(), currentBlock.begin(), currentBlock.end());
                                preservedNetworks++;
                                spdlog::debug("Preserving current network in {}: {}", fileName, currentSSID);
                            } else {
                                removedNetworks++;
                                spdlog::debug("Removing network block from {}", fileName);
                            }
                            inNetworkBlock = false;
                            currentBlock.clear();
                        }
                    } else {
                        // Keep non-network lines (ctrl_interface, etc.)
                        lines.push_back(line);
                    }
                }
                inFile.close();
                
                // Check if any networks were preserved
                if (preservedNetworks == 0) {
                    // No networks to keep - delete the entire file
                    spdlog::debug("No networks to preserve in {}, removing file", fileName);
                    if (deleteFileSafely(configPath)) {
                        removedCount++;
                    } else {
                        success = false;
                    }
                } else {
                    // Write back the filtered config with preserved networks
                    std::ofstream outFile(configPath);
                    if (!outFile.is_open()) {
                        spdlog::error("Could not write wpa_supplicant config: {}", configPath);
                        success = false;
                        continue;
                    }
                    
                    for (const auto &l : lines) {
                        outFile << l << "\n";
                    }
                    outFile.close();
                    
                    updatedCount++;
                    spdlog::debug("Updated wpa_supplicant config {}: removed {} network(s), kept {} network(s)", 
                                fileName, removedNetworks, preservedNetworks);
                }
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("Error while clearing wpa_supplicant configs: {}", e.what());
        success = false;
    }
    
    if (currentSSID.empty()) {
        spdlog::debug("Processed {} wpa_supplicant config file(s), removed {} file(s)", 
                    processedCount, removedCount);
    } else {
        spdlog::debug("Processed {} wpa_supplicant config file(s), updated {} file(s)", 
                    processedCount, updatedCount);
    }
    
    return success;
}

bool ClearWiFiHistory::clearIwdProfiles(const std::string &currentSSID)
{
    spdlog::debug("Clearing iwd network profiles");
    
    const std::string iwdDir = "/var/lib/iwd";
    
    if (!Utils::isFileExists(iwdDir)) {
        spdlog::debug("iwd directory not found: {}", iwdDir);
        return true; // Not an error - just not using iwd
    }
    
    bool success = true;
    int removedCount = 0;
    int skippedCount = 0;
    
    try {
        for (const auto &entry : fs::directory_iterator(iwdDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string filePath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            
            // iwd files are named: SSID.{psk,open,8021x}
            std::string extension = entry.path().extension().string();
            std::string ssid;
            
            if (extension == ".psk" || extension == ".open" || extension == ".8021x") {
                ssid = entry.path().stem().string();
            }
            
            if (ssid.empty()) {
                continue; // Not an iwd network profile
            }
            
            // Skip if this is the currently connected network
            if (!currentSSID.empty() && ssid == currentSSID) {
                spdlog::debug("Skipping current connected network: {}", ssid);
                skippedCount++;
                continue;
            }
            
            spdlog::debug("Removing iwd network profile: {}", fileName);
            if (deleteFileSafely(filePath)) {
                removedCount++;
            } else {
                success = false;
            }
        }
    } catch (const std::exception &e) {
        spdlog::error("Error while clearing iwd profiles: {}", e.what());
        success = false;
    }
    
    spdlog::debug("Removed {} iwd profile(s), kept {} profile(s)", 
                 removedCount, skippedCount);
    
    return success;
}

bool ClearWiFiHistory::clearWiFiJournalLogs()
{
    spdlog::debug("Clearing WiFi-related systemd journal logs");
    
    bool success = true;
    
    // Clear logs for WiFi-related services
    std::vector<std::string> services = {
        "NetworkManager",
        "wpa_supplicant",
        "iwd"
    };
    
    for (const auto &service : services) {
        // Use journalctl to rotate and vacuum logs
        // Note: We use --rotate to close current logs, then --vacuum-time to remove old entries
        int result = Utils::executeCommand("journalctl", 
            {"--rotate", "--unit=" + service});
        
        if (result == 0) {
            // Now vacuum logs older than 1 second (effectively all closed logs)
            Utils::executeCommand("journalctl", 
                {"--vacuum-time=1s", "--unit=" + service});
            spdlog::debug("Cleared journal logs for: {}", service);
        } else {
            spdlog::debug("Could not clear journal logs for: {} (may not be running)", service);
        }
    }
    
    return success;
}

// ========== Helper Methods ==========

bool ClearWiFiHistory::deleteFileSafely(const std::string &filePath)
{
    if (!Utils::isFileExists(filePath)) {
        spdlog::debug("File does not exist, skipping: {}", filePath);
        return true;
    }

    std::error_code ec;
    std::filesystem::remove(filePath, ec);

    if (!ec) {
        spdlog::debug("Successfully deleted: {}", filePath);
        return true;
    } else {
        spdlog::error("Failed to delete: {}: {}", filePath, ec.message());
        return false;
    }
}

bool ClearWiFiHistory::isWirelessConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Convert to lowercase for case-insensitive search
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        
        // Check for common WiFi/wireless indicators
        if (lowerLine.find("type=wifi") != std::string::npos ||
            lowerLine.find("type=802-11-wireless") != std::string::npos ||
            lowerLine.find("[wifi]") != std::string::npos ||
            lowerLine.find("ssid=") != std::string::npos ||
            lowerLine.find("wireless") != std::string::npos) {
            file.close();
            return true;
        }
    }
    
    file.close();
    return false;
}

bool ClearWiFiHistory::isWpaSupplicantConfigFile(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        spdlog::debug("Could not open file to check if it's wpa_supplicant config: {}", filePath);
        return false;
    }
    
    std::string line;
    int maxLinesToCheck = 50; // Don't read entire file, just check first ~50 lines
    int linesChecked = 0;
    
    while (std::getline(file, line) && linesChecked < maxLinesToCheck) {
        linesChecked++;
        
        // Trim whitespace for comparison
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        
        // Look for wpa_supplicant-specific configuration directives
        // These are unique to wpa_supplicant and unlikely in other configs
        if (line.find("ctrl_interface=") == 0 ||
            line.find("ctrl_interface_group=") == 0 ||
            line.find("update_config=") == 0 ||
            line.find("ap_scan=") == 0 ||
            line.find("fast_reauth=") == 0 ||
            line == "network={") {
            file.close();
            return true;
        }
    }
    
    file.close();
    return false;
}

bool ClearWiFiHistory::reloadNetworkManager()
{
    spdlog::debug("Checking if NetworkManager is active");
    
    // First, check if NetworkManager is actually running
    if (!Utils::isNetworkManagerActive()) {
        spdlog::debug("NetworkManager is not active, skipping restart");
        return true; // Not an error - just not using NetworkManager
    }
    
    spdlog::debug("Restarting NetworkManager to apply changes and clear memory cache");
    
    // Restart NetworkManager to completely reload configuration and clear memory cache
    // This is necessary to remove cached WiFi connections from memory
    int result = Utils::executeCommand("systemctl", {"restart", "NetworkManager"});
    
    if (result == 0) {
        spdlog::debug("Successfully restarted NetworkManager");
        return true;
    } else {
        spdlog::warn("Could not restart NetworkManager");
        return true; // Not a critical error
    }
}
