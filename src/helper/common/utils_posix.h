#pragma once

#include <string>
#include <vector>

namespace Utils
{
    // When appendFromStdErr is true, stderr shares stdout's pipe, so error lines are interleaved with stdout in arrival
    // order. Callers must not assume stdout precedes stderr in pOutputStr; parse by matching an expected pattern instead.
    int executeCommand(const std::string &cmd,
                       const std::vector<std::string> &args = std::vector<std::string>(),
                       std::string *pOutputStr = nullptr,
                       bool appendFromStdErr = true);

    // Root-only directory holding everything the helper executes as root.
    std::string getExePath();

    bool locateHelperExecutable(const std::string &name, std::string &outPath);
}
