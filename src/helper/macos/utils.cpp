#include "utils.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <spdlog/spdlog.h>

#include "3rdparty/pstream.h"
#include "firewallonboot.h"

namespace Utils
{

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    std::string cmdLine = cmd;

    for (auto it = args.begin(); it != args.end(); ++it)
    {
        cmdLine += " '";
        for (char c : *it) {
            if (c == '\'') {
                cmdLine += "'\\''";
            } else {
                cmdLine += c;
            }
        }
        cmdLine += "'";
    }

    if (pOutputStr)
    {
        pOutputStr->clear();
    }

    redi::ipstream proc(cmdLine, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
    {
        if (pOutputStr)
        {
            *pOutputStr += line + "\n";
        }
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
    {
        proc.clear();
    }

    if (appendFromStdErr) {
        // read child's stderr
        while (std::getline(proc.err(), line))
        {
            if (pOutputStr)
            {
                *pOutputStr += line + "\n";
            }
        }
    }

    proc.close();
    if (proc.rdbuf()->exited())
    {
        return proc.rdbuf()->status();
    }
    // The child did not exit normally (signal, stop, etc.). Callers compare the return
    // value to 0 to decide success, so a 0 here would falsely report success.
    return -1;
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

bool resolveExePath(const std::string &exePath, const std::string &executable, std::string &outPath)
{
    char *canonicalPath = realpath(exePath.c_str(), NULL);
    if (!canonicalPath) {
        spdlog::warn("Executable not in valid path, ignoring.");
        return false;
    }

#if defined(USE_SIGNATURE_CHECK)
    if (strcmp(canonicalPath, WS_MAC_APP_DIR "/Contents/Helpers") != 0) {
        // Don't execute arbitrary commands, only executables that are in our application directory
        spdlog::warn("Executable not in correct path, ignoring.");
        free(canonicalPath);
        return false;
    }
#endif

    outPath = std::string(canonicalPath) + "/" + executable;
    free(canonicalPath);
    return true;
}

std::vector<std::string> getOpenVpnExeNames()
{
    std::vector<std::string> ret;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(WS_MAC_APP_DIR "/Contents/Helpers", ec), end;
         !ec && it != end; it.increment(ec)) {
        std::string name = it->path().filename().string();
        if (name.find("openvpn") != std::string::npos) {
            ret.push_back(name);
        }
    }
    return ret;
}

void createAppUserAndGroup()
{
    // Always attempt to recreate group/user, even if they exist.

    // Create group
    Utils::executeCommand("dscl", {".", "-create", "/Groups/" WS_PRODUCT_NAME_LOWER});
    // Below attributes are required for group to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Groups/" WS_PRODUCT_NAME_LOWER, "gid", WS_MAC_GID});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/" WS_PRODUCT_NAME_LOWER, "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/" WS_PRODUCT_NAME_LOWER, "GroupMembership", WS_PRODUCT_NAME_LOWER});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/" WS_PRODUCT_NAME_LOWER, "RealName", WS_PRODUCT_NAME " Apps Group"});

    // Create user
    Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "IsHidden", "1"});
    // Below attributes are required for user to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "gid", WS_MAC_GID});
    // For some reason macOS may prompt on this if the user already exists with an uid, so only do this if the user's uid is not set

    std::string uidStr;
    Utils::executeCommand("id", {"-u", WS_PRODUCT_NAME_LOWER}, &uidStr);
    if (uidStr != WS_MAC_UID "\n") {
        spdlog::info("Creating " WS_PRODUCT_NAME_LOWER " user with uid " WS_MAC_UID " (existing uid {})", uidStr);
        Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "uid", WS_MAC_UID});
    }
    Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "RealName", WS_PRODUCT_NAME " Apps User"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/" WS_PRODUCT_NAME_LOWER, "UserShell", "/bin/false"});
}

bool isAppUninstalled()
{
    // App is uninstalled if the application path is no longer valid AND the installer app is not running
    std::error_code ec;
    return !std::filesystem::exists(WS_MAC_APP_DIR, ec) &&
           Utils::executeCommand("pgrep", {"-f", WS_MAC_INSTALLER_BUNDLE_NAME});
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
    std::filesystem::remove_all(Utils::kInstallerStageDir, ec);
    if (ec) {
        spdlog::warn("Failed to remove update dir: {}", ec.message());
    }
    // Note that the following command generally fails with a permission error and does not actually remove the user.
    // It seems on MacOS you need a Secure Token account to delete a user, and even though the privileged helper is running as root, it doesn't have a Secure Token.
    Utils::executeCommand("dscl", {".", "-delete", "/Users/" WS_PRODUCT_NAME_LOWER});
    Utils::executeCommand("dscl", {".", "-delete", "/Groups/" WS_PRODUCT_NAME_LOWER});
}

bool hasWhitespaceInString(const std::string &str)
{
    return str.find_first_of(" \n\r\t") != std::string::npos;
}

std::string getExePath()
{
    return WS_MAC_APP_DIR "/Contents/Helpers";
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
