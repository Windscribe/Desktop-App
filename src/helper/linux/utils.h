#pragma once

#include <string>
#include <vector>

#include "../common/helper_commands.h"
#include "../common/utils_posix.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

namespace Utils
{
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
};
