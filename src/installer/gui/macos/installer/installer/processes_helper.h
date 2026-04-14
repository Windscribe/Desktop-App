#pragma once

#include <vector>
#include <string>

// util class for find processes by name
class ProcessesHelper
{
public:
    ProcessesHelper();

    std::vector<pid_t> getPidsByProcessname(const char *name);
    bool isProcessFinished(pid_t pid) const;
};

