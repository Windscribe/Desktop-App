#include "utils.h"

#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../common/spawn_posix.h"
#include "../common/validation_posix.h"

namespace Utils
{

int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    // Utilities such as nmcli and ping translate their output under a non-English locale, which breaks
    // the callers that parse it. C.UTF-8 rather than C so non-ASCII connection names survive nmcli's
    // charset conversion; LANGUAGE is unset because older glibc lets it override LC_ALL for messages.
    return Spawn::runCommand(cmd, args, pOutputStr, appendFromStdErr, {{"LC_ALL", "C.UTF-8"}}, {"LANGUAGE"});
}

bool locateHelperExecutable(const std::string &name, std::string &outPath)
{
    return resolveExePath(getExePath(), name, outPath);
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

bool isNetworkManagerActive()
{
    // Same semantics as the client's LinuxUtils::isNetworkManagerActive(): systemctl answers from PID 1
    // and should never block; if it somehow does (timeout exits 124), err toward active so we don't skip
    // NetworkManager-backed operations over a transient hiccup.
    const int ret = Utils::executeCommand("timeout", {"2", "systemctl", "--quiet", "is-active", "NetworkManager.service"});
    return ret == 0 || ret == 124;
}

} // namespace Utils
