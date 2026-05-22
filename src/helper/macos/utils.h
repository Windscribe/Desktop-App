#pragma once

#include <string>
#include <vector>

namespace Utils
{
    // Root-owned location where installerStageAndVerify drops the verified installer.app
    // and where installerCleanupStaged / deleteSelf wipe it from. Hoisted into a shared
    // constant so a branding rename touches one site instead of three.
    inline constexpr const char *kInstallerStageDir = "/Library/Application Support/Windscribe/update";

    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr,
                       bool appendFromStdErr = true);

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);

    bool isFileExists(const std::string &name);

    // Canonicalize exePath and append `executable`, validating the result lives inside the
    // app bundle. Returns true on success and writes the absolute path to outPath.
    bool resolveExePath(const std::string &exePath, const std::string &executable, std::string &outPath);

    // get list of openvpn exe names from package
    std::vector<std::string> getOpenVpnExeNames();

    void createAppUserAndGroup();

    // check if the app has been uninstalled
    bool isAppUninstalled();

    // delete this helper app and the cli symlink
    void deleteSelf();

    // check if string has whitespace
    bool hasWhitespaceInString(const std::string &str);

    // get root path for app utils
    std::string getExePath();

    // check if a string is a canonical SCDynamicStore DNS entry of the form
    // (State|Setup):/Network/Service/<id>/DNS, where <id> is alphanumeric/._-
    bool isValidDnsDynamicStoreEntry(const std::string &entry);

    bool isPortListening(unsigned int port, int maxRetries = 10, int delayMs = 100);
};
