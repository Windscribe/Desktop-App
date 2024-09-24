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

#include "logger.h"

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
        Logger::instance().out("Executable not in valid path, ignoring.");
        return "";
    }

// check only for release build
#ifdef NDEBUG
    if (std::string(canonicalPath).rfind("/opt/windscribe", 0) != 0 && std::string(canonicalPath).rfind("/usr/lib/opt/windscribe", 0) != 0) {
        // Don't execute arbitrary commands, only executables that are in our application directory
        Logger::instance().out("Executable not in correct path, ignoring.");
        free(canonicalPath);
        return "";
    }
#endif

    std::string fullCmd = std::string(canonicalPath) + "/" + executable + " " + arguments;
    Logger::instance().out("Resolved command: %s", fullCmd.c_str());
    free(canonicalPath);

    if (fullCmd.find_first_of(";|&`") != std::string::npos) {
        // Don't execute commands with dangerous pipes or delimiters
        Logger::instance().out("Executable command contains invalid characters, ignoring.");
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
        return "/etc/windscribe/update-systemd-resolved";
    case kResolvConf:
        return "/etc/windscribe/update-resolv-conf";
    case kNetworkManager:
        return "/etc/windscribe/update-network-manager";
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
    Logger::instance().out("Wireless MAC for network: %s: %s", network.c_str(), output.c_str());
    if (ret == 0 && !output.empty() && output.rfind("preserve", 0) == std::string::npos) {
        return true;
    }
    ret = Utils::executeCommand("nmcli", {"-g", "802-3-ethernet.cloned-mac-address", "connection", "show", network.c_str()}, &output);
    Logger::instance().out("Wired MAC for network: %s: %s", network.c_str(), output.c_str());
    if (ret == 0 && !output.empty() && output.rfind("preserve", 0) == std::string::npos) {
        return true;
    }
    return false;
}

bool resetMacAddresses(const std::string &ignoreNetwork)
{
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
        is2 >> state >> name;

        // skip ignored network
        if (name == "lo" || name == ignoreNetwork) {
            continue;
        }

        if (!isMacAddressSpoofed(name.c_str())) {
            continue;
        }

        Utils::executeCommand("nmcli", {"connection", "modify", name.c_str(), "wifi.cloned-mac-address", "preserve"}, &output);
        Utils::executeCommand("nmcli", {"connection", "modify", name.c_str(), "ethernet.cloned-mac-address", "preserve"}, &output);

        Logger::instance().out("Reset MAC addresses: %s (state = %s)", name.c_str(), state.c_str());

        if (state == "activated") {
            Utils::executeCommand("nmcli", {"connection", "up", name.c_str()});
        }
    }
    return true;
}

} // namespace Utils
