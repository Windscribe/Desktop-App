#pragma once

#include <string>
#include <vector>

namespace Utils
{
    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr,
                       bool appendFromStdErr = true);

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);

    bool isFileExists(const std::string &name);

    // combine exe path, exe, and arguments
    std::string getFullCommand(const std::string &exePath, const std::string &executable, const std::string &arguments);
    std::string getFullCommandAsUser(const std::string &user, const std::string &exePath, const std::string &executable, const std::string &arguments);

    // get list of openvpn exe names from package
    std::vector<std::string> getOpenVpnExeNames();

    // create the system group and user 'windscribe'
    void createWindscribeUserAndGroup();

    // check if the app has been uninstalled
    bool isAppUninstalled();

    // delete this helper app and the cli symlink
    void deleteSelf();

    // check if string has whitespace
    bool hasWhitespaceInString(const std::string &str);

    // get root path for app utils
    std::string getExePath();

    // check if a string is a valid address
    std::string normalizeAddress(const std::string &address);
    bool isValidIpAddress(const std::string &address);
    bool isValidIpv4Address(const std::string &address);
    bool isValidIpv6Address(const std::string &address);
    bool isValidDomain(const std::string &address);
};
