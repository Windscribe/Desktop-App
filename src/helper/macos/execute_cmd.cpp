#include "execute_cmd.h"
#include <spdlog/spdlog.h>

void ExecuteCmd::execute(const std::string &cmd, const std::string &cwd)
{
    std::string fullCmd = cmd;
    if (!cwd.empty()) {
        fullCmd = "cd \"" + cwd + "\" && " + cmd;
    }

    std::string daemonCmd = fullCmd + " &";
    int result = system(daemonCmd.c_str());
    if (result != 0) {
        spdlog::warn("system() call failed with result: {}", result);
    }
}
