#include "utils.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <ifaddrs.h>
#include <net/if.h>
#include <set>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "../common/io_posix.h"
#include "../common/process_kill_posix.h"
#include "../common/spawn_posix.h"
#include "../common/validation_posix.h"
#include "directory_mac.h"
#include "firewallonboot.h"
#include "utils/executable_signature/executable_signature.h"

namespace Utils
{

std::vector<std::string> getAwdlP2pInterfaces()
{
    std::vector<std::string> ret;
    std::set<std::string> upInterfaces;
    struct ifaddrs *interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0) {
        for (struct ifaddrs *ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_name != nullptr && (ifa->ifa_flags & IFF_UP)) {
                upInterfaces.insert(ifa->ifa_name);
            }
        }
        freeifaddrs(interfaces);
    } else {
        spdlog::error("getAwdlP2pInterfaces: getifaddrs failed: {}", IO::strerror(errno));
    }

    for (const std::string &name : upInterfaces) {
        if (name.rfind("p2p", 0) == 0 || name.rfind("awdl", 0) == 0 || name.rfind("llw", 0) == 0) {
            // Validate before interpolating into pf rule text ("pass quick on <iface>"), same as the
            // client-supplied vpnInterfaceName and the Linux getHotspotAdapter path: a name carrying
            // pf grammar characters must not reach the rule body.
            if (!Validation::isValidInterfaceName(name)) {
                spdlog::error("getAwdlP2pInterfaces: invalid interface name \"{}\", ignoring", name);
                continue;
            }
            ret.push_back(name);
        }
    }
    return ret;
}

int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    return Spawn::runCommand(cmd, args, pOutputStr, appendFromStdErr);
}

bool locateHelperExecutable(const std::string &name, std::string &outPath)
{
    outPath = getExePath() + "/" + name;
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(outPath)) {
        spdlog::error("{} executable signature incorrect: {}", name, sigCheck.lastError());
        return false;
    }
    return true;
}


size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos)
{
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);
    return data.find(toSearch, pos);
}

bool isFileExists(const std::string &name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

bool ensureVendorDir()
{
    // A symlink would be honoured by everything below: create_directories() sees it as existing,
    // chmod() applies to its target, and root would then write and execute through it.
    struct stat st;
    if (::lstat(kVendorDir, &st) == 0 && !S_ISDIR(st.st_mode)) {
        spdlog::error("\"{}\" exists and is not a directory; refusing to use it", kVendorDir);
        return false;
    }

    std::error_code ec;
    {
        // Without this the mode would come from the ambient umask.
        UmaskGuard guard(022);
        std::filesystem::create_directories(kVendorDir, ec);
    }
    if (ec) {
        spdlog::error("Failed to create \"{}\": {}", kVendorDir, ec.message());
        return false;
    }
    // Ownership before mode, so a directory that pre-existed with another owner is not handed
    // exclusive write to itself by the chmod.
    if (::lchown(kVendorDir, 0, 0) != 0 || ::chmod(kVendorDir, 0755) != 0) {
        spdlog::error("Failed to secure \"{}\": {}", kVendorDir, IO::strerror(errno));
        return false;
    }
    return true;
}

std::vector<std::string> getOpenVpnExeNames()
{
    // A fixed name, not a directory listing, so which processes killOpenVPN stops never depends on the files on disk.
    return { WS_PRODUCT_NAME_LOWER "openvpn" };
}


bool isAppUninstalled()
{
    // App is uninstalled if the application path is no longer valid AND the installer app is not running
    std::error_code ec;
    return !std::filesystem::exists(WS_MAC_APP_DIR, ec) &&
           !ProcessKill::isAnyExecutablePathContaining(WS_MAC_INSTALLER_BUNDLE_NAME);
}

void deleteSelf()
{
    FirewallOnBootManager::instance().setEnabled(false);

    Utils::executeCommand("launchctl", {"remove", "/Library/LaunchDaemons/" WS_MAC_HELPER_BUNDLE_ID ".plist"});
    std::error_code ec;
    std::filesystem::remove("/Library/LaunchDaemons/" WS_MAC_HELPER_BUNDLE_ID ".plist", ec);
    if (ec) {
        spdlog::warn("Failed to remove helper plist: {}", ec.message());
    }
    std::filesystem::remove("/Library/PrivilegedHelperTools/" WS_MAC_HELPER_BUNDLE_ID, ec);
    if (ec) {
        spdlog::warn("Failed to remove privileged helper: {}", ec.message());
    }
    std::filesystem::remove("/usr/local/bin/" WS_CLI_EXECUTABLE_NAME, ec);
    if (ec) {
        spdlog::warn("Failed to remove CLI symlink: {}", ec.message());
    }
    // One sweep: update/, payload/, bin/ and Frameworks/ all live under the vendor directory.
    std::filesystem::remove_all(Utils::kVendorDir, ec);
    if (ec) {
        spdlog::warn("Failed to remove vendor dir: {}", ec.message());
    }
    std::filesystem::remove_all(Utils::kArchiveTempDir, ec);
    if (ec) {
        spdlog::warn("Failed to remove archive temp dir: {}", ec.message());
    }
    // The following deletion generally fails with a permission error: macOS appears to require a Secure Token account
    // to delete a user, and the privileged helper has no Secure Token even though it runs as root.
    Directory::Node node;
    if (node) {
        node.deleteRecord(Directory::RecordType::User, WS_PRODUCT_NAME_LOWER);
        node.deleteRecord(Directory::RecordType::Group, WS_PRODUCT_NAME_LOWER);
    }
}

bool hasWhitespaceInString(const std::string &str)
{
    return str.find_first_of(" \n\r\t") != std::string::npos;
}

std::string getExePath()
{
    return Utils::kHelperBinDir;
}

bool isValidDnsDynamicStoreEntry(const std::string &entry)
{
    static const std::string kStatePrefix = "State:/Network/Service/";
    static const std::string kSetupPrefix = "Setup:/Network/Service/";
    static const std::string kSuffix = "/DNS";

    std::string id;
    if (entry.rfind(kStatePrefix, 0) == 0) {
        id = entry.substr(kStatePrefix.size());
    } else if (entry.rfind(kSetupPrefix, 0) == 0) {
        id = entry.substr(kSetupPrefix.size());
    } else {
        return false;
    }

    if (id.size() <= kSuffix.size() ||
        id.compare(id.size() - kSuffix.size(), kSuffix.size(), kSuffix) != 0) {
        return false;
    }
    id.erase(id.size() - kSuffix.size());

    if (id.empty()) {
        return false;
    }

    for (char c : id) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

bool isPortListening(unsigned int port, int maxRetries, int delayMs)
{
    auto startTime = std::chrono::steady_clock::now();

    for (int attempt = 0; attempt < maxRetries; attempt++) {
        std::string output;
        int result = executeCommand("lsof", {"-nP", "-iTCP:" + std::to_string(port), "-sTCP:LISTEN"}, &output);

        if (result == 0 && !output.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            spdlog::info("Port {} is listening after {} attempts ({}ms)", port, attempt + 1, elapsed);
            return true;
        }

        if (attempt < maxRetries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    spdlog::warn("Port {} not listening after {} attempts ({}ms)", port, maxRetries, elapsed);
    return false;
}

} // namespace Utils
