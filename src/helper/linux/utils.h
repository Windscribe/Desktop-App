#pragma once

#include <string>
#include <vector>
#include "../common/helper_commands.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

namespace Utils
{
    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    // When appendFromStdErr is true, stderr is merged into stdout via a subshell redirect, so error
    // lines are interleaved with stdout in arrival order rather than appended after it. Callers must
    // not assume stdout precedes stderr in pOutputStr (e.g. do not treat the first line as data);
    // parse by matching an expected marker/pattern instead.
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr, bool appendFromStdErr = true);

    // true if the NetworkManager systemd unit is currently active
    bool isNetworkManagerActive();

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);

    bool isFileExists(const std::string &name);

    // Canonicalize exePath and append `executable`, validating the result lives inside the
    // install dir. Returns true on success and writes the absolute path to outPath.
    bool resolveExePath(const std::string &exePath, const std::string &executable, std::string &outPath);

    // get list of openvpn exe names from package
    std::vector<std::string> getOpenVpnExeNames();

    // get dns script associated with a particular dns manager
    std::string getDnsScript(CmdDnsManager mgr);

    // check if string has whitespace
    bool hasWhitespaceInString(const std::string &str);

    // get root path for app utils
    std::string getExePath();

    // resets MAC address to original (hw) address, optionally ignoring one interface
    bool resetMacAddresses(const std::string &ignoreNetwork = "");

    // MAC address utilities
    bool isMacAddressSpoofed(const std::string &network);
};
