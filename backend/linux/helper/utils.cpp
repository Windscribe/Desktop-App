#include "utils.h"
#include "3rdparty/pstream.h"

#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <skyr/core/parse.hpp>
#include <skyr/core/serialize.hpp>
#include <skyr/url.hpp>
#include <sstream>
#include <sys/stat.h>
#include <spdlog/spdlog.h>

namespace Utils
{

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    std::string cmdLine = cmd;

    for (auto it = args.begin(); it != args.end(); ++it) {
        cmdLine += " \"";
        cmdLine += *it;
        cmdLine += "\"";
    }

    if (pOutputStr) {
        pOutputStr->clear();
    }

    redi::ipstream proc(cmdLine, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line)) {
        if (pOutputStr) {
            *pOutputStr += line + "\n";
        }
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail()) {
        proc.clear();
    }

    if (appendFromStdErr) {
        // read child's stderr
        while (std::getline(proc.err(), line)) {
            if (pOutputStr) {
                *pOutputStr += line + "\n";
            }
        }
    }

    proc.close();
    if (proc.rdbuf()->exited()) {
        return proc.rdbuf()->status();
    }
    return 0;
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

std::string getFullCommand(const std::string &exePath, const std::string &executable, const std::string &arguments)
{
    char *canonicalPath = realpath(exePath.c_str(), NULL);
    if (!canonicalPath) {
        spdlog::warn("Executable not in valid path, ignoring.");
        return "";
    }

// check only for release build
#ifdef NDEBUG
    if (std::string(canonicalPath).rfind("/opt/windscribe", 0) != 0 && std::string(canonicalPath).rfind("/usr/lib/opt/windscribe", 0) != 0) {
        // Don't execute arbitrary commands, only executables that are in our application directory
        spdlog::warn("Executable not in correct path, ignoring.");
        free(canonicalPath);
        return "";
    }
#endif

    std::string fullCmd = std::string(canonicalPath) + "/" + executable + " " + arguments;
    spdlog::debug("Resolved command: {}", fullCmd);
    free(canonicalPath);

    if (fullCmd.find_first_of(";|&`") != std::string::npos) {
        // Don't execute commands with dangerous pipes or delimiters
        spdlog::warn("Executable command contains invalid characters, ignoring.");
        return "";
    }

    return fullCmd;
}

std::string getFullCommandAsUser(const std::string &user, const std::string &exePath, const std::string &executable, const std::string &arguments)
{
    return "sudo -u " + user + " " + getFullCommand(exePath, executable, arguments);
}

std::vector<std::string> getOpenVpnExeNames()
{
    std::vector<std::string> ret;
    std::string list;
    std::string item;
    int rc = 0;

    rc = Utils::executeCommand("ls", {"/opt/windscribe"}, &list);
    if (rc != 0) {
        return ret;
    }

    std::istringstream stream(list);
    while (std::getline(stream, item)) {
        if (item.find("openvpn") != std::string::npos) {
            ret.push_back(item);
        }
    }
    return ret;
}

std::string getDnsScript(CmdDnsManager mgr)
{
    switch(mgr) {
    case kSystemdResolved:
        return "/opt/windscribe/scripts/update-systemd-resolved";
    case kResolvConf:
        return "/opt/windscribe/scripts/update-resolv-conf";
    case kNetworkManager:
        return "/opt/windscribe/scripts/update-network-manager";
    default:
        return "";
    }
}

void createWindscribeUserAndGroup()
{
    bool exists = !Utils::executeCommand("id windscribe");
    if (exists) {
        return;
    }

    // Create group
    Utils::executeCommand("groupadd", {"windscribe"});
    // Create user
    Utils::executeCommand("useradd", {"-r", "-g", "windscribe", "-s", "/bin/false", "windscribe"});
}

bool hasWhitespaceInString(const std::string &str)
{
    return str.find_first_of(" \n\r\t") != std::string::npos;
}

std::string getExePath()
{
    return "/opt/windscribe";
}

bool isValidIpAddress(const std::string &address)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) != 0;
}

bool isValidDomain(const std::string &address)
{
    if (isValidIpAddress(address)) {
        return false;
    }

    auto domain = skyr::parse_host(address);
    if (!domain) {
        return false;
    }

    return true;
}

std::string normalizeAddress(const std::string &address)
{
    std::string addr = address;

    if (isValidIpAddress(address)) {
        return addr;
    }

    if (isValidDomain(address)) {
        return addr;
    }

    auto url = skyr::parse(address);
    if (!url) {
        return "";
    }

    return skyr::serialize(url.value());
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
    std::string output;
    Utils::executeCommand("cat", {"/sys/class/net/" + ifname + "/address"}, &output);
    // Remove trailing whitespace
    output.erase(output.find_last_not_of(" \n\r\t") + 1);
    return output;
}
#endif

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

        Utils::executeCommand("nmcli", {"connection", "modify", name.c_str(), "wifi.cloned-mac-address", "preserve"}, &output);
        Utils::executeCommand("nmcli", {"connection", "modify", name.c_str(), "ethernet.cloned-mac-address", "preserve"}, &output);

        spdlog::info("Reset MAC addresses: {} (state = {})", name, state);

        if (state == "activated") {
            Utils::executeCommand("nmcli", {"connection", "up", name.c_str()});
        }
    }
#endif
    return true;
}

} // namespace Utils
