#ifndef Utils_h
#define Utils_h

#include <string>
#include <vector>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

namespace Utils
{
    // execute cmd with args and return output from stdout and stderror to pOutputStr (if pOutputStr != NULL)
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr, bool appendFromStdErr = true);

    // find case insensitive sub string in a given substring
    size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos = 0);
};

#endif
