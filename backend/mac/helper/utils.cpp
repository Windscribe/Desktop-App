#include "utils.h"

#include <arpa/inet.h>
#include <skyr/core/parse.hpp>
#include <skyr/core/serialize.hpp>
#include <skyr/url.hpp>
#include <sstream>
#include <sys/stat.h>
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
        cmdLine += " \"";
        cmdLine += *it;
        cmdLine += "\"";
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

#if defined(USE_SIGNATURE_CHECK)
    if (std::string(canonicalPath).rfind("/Applications/Windscribe.app", 0) != 0) {
        // Don't execute arbitrary commands, only executables that are in our application directory
        spdlog::warn("Executable not in correct path, ignoring.");
        free(canonicalPath);
        return "";
    }
#endif

    std::string fullCmd = std::string(canonicalPath) + "/" + executable + " " + arguments;
    spdlog::debug("Resolved command: %s", fullCmd);
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

    rc = Utils::executeCommand("ls", {"/Applications/Windscribe.app/Contents/Helpers"}, &list);
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

void createWindscribeUserAndGroup()
{
    // Always attempt to recreate group/user, even if they exist.

    // Create group
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe"});
    // Below attributes are required for group to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "gid", "518"}); // Arbitrary number
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "GroupMembership", "windscribe"});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "RealName", "Windscribe Apps Group"});

    // Create user
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "IsHidden", "1"});
    // Below attributes are required for user to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "gid", "518"}); // From above
    // For some reason macOS may prompt on this if the user already exists with an uid, so only do this if the user's uid is not set

    std::string uidStr;
    Utils::executeCommand("id", {"-u", "windscribe"}, &uidStr);
    if (uidStr != "1639\n") {
        spdlog::info("Creating windscribe user with uid 1639 (existing uid {})", uidStr);
        Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "uid", "1639"}); // Arbitrary number
    }
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "RealName", "Windscribe Apps User"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "UserShell", "/bin/false"});
}

bool isAppUninstalled()
{
    // App is uninstalled if the application path is no longer valid AND the installer app is not running
    return Utils::executeCommand("ls", {"/Applications/Windscribe.app"}) && Utils::executeCommand("pgrep", {"-f", "WindscribeInstaller.app"});
}

void deleteSelf()
{
    FirewallOnBootManager::instance().setEnabled(false);

    Utils::executeCommand("launchctl", {"remove", "/Library/LaunchDaemons/com.windscribe.helper.macos.plist"});
    Utils::executeCommand("rm", {"/Library/LaunchDaemons/com.windscribe.helper.macos.plist"});
    Utils::executeCommand("rm", {"/Library/PrivilegedHelperTools/com.windscribe.helper.macos"});
    Utils::executeCommand("rm", {"/usr/local/bin/windscribe-cli"});
    // Note that the following command generally fails with a permission error and does not actually remove the user.
    // It seems on MacOS you need a Secure Token account to delete a user, and even though the privileged helper is running as root, it doesn't have a Secure Token.
    Utils::executeCommand("dscl", {".", "-delete", "/Users/windscribe"});
    Utils::executeCommand("dscl", {".", "-delete", "/Groups/windscribe"});
}

bool hasWhitespaceInString(const std::string &str)
{
    return str.find_first_of(" \n\r\t") != std::string::npos;
}

std::string getExePath()
{
    return "/Applications/Windscribe.app/Contents/Helpers";
}

bool isValidIpAddress(const std::string &address)
{
    return isValidIpv4Address(address) || isValidIpv6Address(address);
}

bool isValidIpv4Address(const std::string &address)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) != 0;
}

bool isValidIpv6Address(const std::string &address)
{
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, address.c_str(), &(sa.sin6_addr)) != 0;
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

} // namespace Utils
