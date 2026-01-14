#pragma once

#include <string>

class ExecuteCmd
{
public:
    static ExecuteCmd &instance()
    {
        static ExecuteCmd i;
        return i;
    }

    void execute(const std::string &cmd, const std::string &cwd = "");

private:
    ExecuteCmd() = default;
};
