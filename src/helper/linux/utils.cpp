#include "utils.h"
#include "3rdparty/pstream.h"
#include "../common/validation_posix.h"

#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spdlog/spdlog.h>

namespace Utils
{

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    std::string cmdLine = cmd;

    for (auto it = args.begin(); it != args.end(); ++it) {
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

    if (pOutputStr) {
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
    if (proc.rdbuf()->exited()) {
        const int status = proc.rdbuf()->status();
        // status() is the raw wait-status from waitpid (e.g. exit code 2 -> 512), which is
        // misleading when logged. Normalize to the real exit code so callers and logs see what
        // the command actually returned.
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        // Terminated by a signal, so report abnormal rather than a status callers might read as an
        // exit code.
        return -1;
    }
    // The child did not exit normally (signal, stop, etc.). Callers compare the return value to 0
    // to decide success, so a 0 here would falsely report success.
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

// check only for non-dev builds
#ifndef WINDSCRIBE_DEV_MODE
    if (strcmp(canonicalPath, WS_LINUX_INSTALL_DIR) != 0 && strcmp(canonicalPath, "/usr/lib" WS_LINUX_INSTALL_DIR) != 0) {
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
    for (std::filesystem::directory_iterator it(WS_LINUX_INSTALL_DIR, ec), end; !ec && it != end; it.increment(ec)) {
        std::string name = it->path().filename().string();
        if (name.find("openvpn") != std::string::npos) {
            ret.push_back(name);
        }
    }
    return ret;
}

std::string getDnsScript(CmdDnsManager mgr)
{
    switch(mgr) {
    case kSystemdResolved:
        return WS_LINUX_INSTALL_DIR "/scripts/update-systemd-resolved";
    case kResolvConf:
        return WS_LINUX_INSTALL_DIR "/scripts/update-resolv-conf";
    case kNetworkManager:
        return WS_LINUX_INSTALL_DIR "/scripts/update-network-manager";
    default:
        return "";
    }
}

bool hasWhitespaceInString(const std::string &str)
{
    return str.find_first_of(" \n\r\t") != std::string::npos;
}

std::string getExePath()
{
    return WS_LINUX_INSTALL_DIR;
}

bool isMacAddressSpoofed(const std::string &network)
{
    std::string output;
    int ret = Utils::executeCommand("nmcli", {"-g", "802-11-wireless.cloned-mac-address", "connection", "show", network.c_str()}, &output);
    spdlog::info("Wireless MAC for network: {}: {}", network, output);
    if (ret == 0 && !output.empty() && output.rfind("preserve", 0) == std::string::npos) {
        return true;
    }
    ret = Utils::executeCommand("nmcli", {"-g", "802-3-ethernet.cloned-mac-address", "connection", "show", network.c_str()}, &output);
    spdlog::info("Wired MAC for network: {}: {}", network.c_str(), output);
    if (ret == 0 && !output.empty() && output.rfind("preserve", 0) == std::string::npos) {
        return true;
    }
    return false;
}

#ifdef CLI_ONLY
static std::vector<std::string> getInterfaceNames()
{
    std::string output;
    std::string line;
    std::vector<std::string> interfaces;

    Utils::executeCommand("ip", {"link"}, &output);

    std::stringstream is(output);
    while (std::getline(is, line)) {
        if (line.empty() || line[0] == ' ') {
            continue;
        }

        // Interface name is between the first space and the next colon
        size_t start = line.find(' ');
        if (start == std::string::npos) {
            continue;
        }
        start++; // Move past the space
        size_t end = line.find(':', start);
        if (end == std::string::npos) {
            continue;
        }
        interfaces.push_back(line.substr(start, end - start));
    }

    return interfaces;
}

static std::string getHwMac(const std::string &ifname)
{
    if (!Validation::isValidInterfaceName(ifname)) {
        spdlog::error("Invalid interface name in getHwMac: {}", ifname);
        return "";
    }

    std::string output;
    Utils::executeCommand("ethtool", {"-P", ifname}, &output);

    // Address is between the second space and new line
    size_t start = output.find(' ', output.find(' ') + 1);
    if (start == std::string::npos) {
        return "";
    }
    start++; // Move past the space
    size_t end = output.find('\n', start);
    if (end == std::string::npos) {
        return "";
    }

    return output.substr(start, end - start);
}

static std::string getCurrentMac(const std::string &ifname)
{
    if (!Validation::isValidInterfaceName(ifname)) {
        spdlog::error("Invalid interface name in getCurrentMac: {}", ifname);
        return "";
    }

    std::ifstream addrFile("/sys/class/net/" + ifname + "/address");
    std::string output;
    if (addrFile) {
        std::getline(addrFile, output);
    }
    // Remove trailing whitespace
    output.erase(output.find_last_not_of(" \n\r\t") + 1);
    return output;
}
#endif

bool isNetworkManagerActive()
{
    // Same semantics as the client's LinuxUtils::isNetworkManagerActive(): systemctl answers from PID 1
    // and should never block; if it somehow does (timeout exits 124), err toward active so we don't skip
    // NetworkManager-backed operations over a transient hiccup.
    const int ret = Utils::executeCommand("timeout", {"2", "systemctl", "--quiet", "is-active", "NetworkManager.service"});
    return ret == 0 || ret == 124;
}

bool resetMacAddresses(const std::string &ignoreNetwork)
{
#ifdef CLI_ONLY
    std::vector<std::string> interfaces = getInterfaceNames();

    for (const auto interface : interfaces) {
        // skip ignored network
        if (interface == ignoreNetwork) {
            continue;
        }

        std::string hwAddr = getHwMac(interface);
        std::string curAddr = getCurrentMac(interface);
        if (hwAddr != curAddr && hwAddr != "not set") {
            spdlog::info("Resetting MAC spoofing on {} (hw: {}, cur: {})", interface, hwAddr, curAddr);
            // Must bring down interface to change the MAC address
            Utils::executeCommand("ip", {"link", "set", "dev", interface, "down"});
            Utils::executeCommand("ip", {"link", "set", "dev", interface, "address", hwAddr});
            Utils::executeCommand("ip", {"link", "set", "dev", interface, "up"});
        }
    }
#else
    // Spoofs are applied via NetworkManager profiles; without the daemon there is nothing to reset
    // and every nmcli call below would just fail (e.g. NM-less distros, this runs at every helper boot).
    if (!Utils::isNetworkManagerActive()) {
        spdlog::info("resetMacAddresses skipped: NetworkManager is not active");
        return true;
    }

    std::string output;
    std::string line;
    bool firstline = true;
    Utils::executeCommand("nmcli", {"--fields", "state,name", "connection", "show"}, &output);

    std::stringstream is(output);
    while (std::getline(is, line)) {
        // skip labels
        if (firstline) {
            firstline = false;
            continue;
        }

        std::stringstream is2(line);
        std::string name;
        std::string state;
        is2 >> state;
        // The entire rest of the line is the name, which may contain spaces.  Trim spaces before/after the name.
        getline(is2, name);
        name.erase(0, name.find_first_not_of(' '));
        name.erase(name.find_last_not_of(' ') + 1);

        // skip ignored network
        if (name == "lo" || name == ignoreNetwork) {
            continue;
        }

        if (!isMacAddressSpoofed(name.c_str())) {
            continue;
        }

        // Use an explicit "id" selector so a connection literally named "id"/"uuid"/"path" can't be
        // reinterpreted by nmcli as a selector keyword that consumes the following token.
        Utils::executeCommand("nmcli", {"connection", "modify", "id", name.c_str(), "wifi.cloned-mac-address", "preserve"}, &output);
        Utils::executeCommand("nmcli", {"connection", "modify", "id", name.c_str(), "ethernet.cloned-mac-address", "preserve"}, &output);

        spdlog::info("Reset MAC addresses: {} (state = {})", name, state);

        if (state == "activated") {
            Utils::executeCommand("nmcli", {"connection", "up", "id", name.c_str()});
        }
    }
#endif
    return true;
}

} // namespace Utils
