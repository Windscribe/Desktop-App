#ifndef Utils_h
#define Utils_h

#include <string>
#include <vector>

namespace Utils
{
    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr, bool appendFromStdErr = true);

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);

    // combine exe path, exe, and arguments
    std::string getFullCommand(const std::string &exePath, const std::string &executable, const std::string &arguments);

    // get list of openvpn exe names from package
    std::vector<std::string> getOpenVpnExeNames();
};

#endif
