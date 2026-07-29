#include "utils.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "3rdparty/pstream.h"
#include "firewallonboot.h"
#include "../common/io_posix.h"
#include "../common/validation_posix.h"

namespace Utils
{

std::vector<std::string> getAwdlP2pInterfaces()
{
    std::vector<std::string> ret;
    std::string output;
    executeCommand("ifconfig", {"-l", "-u"}, &output);

    std::istringstream stream(output);
    std::string name;
    while (stream >> name) {
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

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    // Single-quote the program too, not just the args — helper binaries live under
    // "Application Support" and an unquoted path word-splits in the shell.
    auto quote = [](const std::string &s) {
        std::string out = "'";
        for (char c : s) {
            if (c == '\'') {
                out += "'\\''";
            } else {
                out += c;
            }
        }
        out += "'";
        return out;
    };

    std::string cmdLine = quote(cmd);
    for (auto it = args.begin(); it != args.end(); ++it) {
        cmdLine += " " + quote(*it);
    }

    if (pOutputStr)
    {
        pOutputStr->clear();
    }

    // Merge stderr into stdout via a subshell and read only that one pipe: draining two pipes
    // separately can deadlock if the child fills the one we aren't reading. The ( ) wrapper makes
    // the redirect cover every stage of a pipeline, not just the last, and leaves the exit status
    // unchanged.
    if (appendFromStdErr) {
        cmdLine = "( " + cmdLine + " ) 2>&1";
    } else {
        cmdLine = "( " + cmdLine + " ) 2>/dev/null";
    }

    redi::ipstream proc(cmdLine, redi::pstreams::pstdout);
    std::string line;
    // read child's stdout (with stderr merged in when requested)
    while (std::getline(proc.out(), line)) {
        if (pOutputStr) {
            *pOutputStr += line + "\n";
        }
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail()) {
        proc.clear();
    }

    proc.close();
    if (proc.rdbuf()->exited())
    {
        const int status = proc.rdbuf()->status();
        // status() is the raw waitpid() value; return the decoded exit code so callers that log the
        // result see the real code (e.g. 1) rather than the encoded status (e.g. 256).
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        // Terminated by a signal, so report abnormal rather than a status callers might read as an
        // exit code.
        return -1;
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
    // A fixed name, not a directory listing. killOpenVPN turns these into a `pkill -f` pattern,
    // i.e. a regex, so enumerating a directory made the pattern depend on filenames on disk.
    return { WS_PRODUCT_NAME_LOWER "openvpn" };
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
    // One sweep: update/, payload/, bin/ and Frameworks/ all live under the vendor directory.
    std::filesystem::remove_all(Utils::kVendorDir, ec);
    if (ec) {
        spdlog::warn("Failed to remove vendor dir: {}", ec.message());
    }
    std::filesystem::remove_all(Utils::kArchiveTempDir, ec);
    if (ec) {
        spdlog::warn("Failed to remove archive temp dir: {}", ec.message());
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
