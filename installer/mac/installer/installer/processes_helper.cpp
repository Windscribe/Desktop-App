#include "processes_helper.h"
#include <signal.h>
#include <libproc.h>

ProcessesHelper::ProcessesHelper()
{
}

std::vector<pid_t> ProcessesHelper::getPidsByProcessname(const char *name)
{
    std::string cmd = "pgrep ";
    cmd.append(name);

    std::vector<pid_t> ret;
    FILE *proc = popen(cmd.c_str(), "r");
    char result[16];
    bzero(result, sizeof(result));
    while (fgets(result, sizeof(result), proc) != nullptr)
    {
        pid_t pid = atoi(result);
        if (pid != 0)
        {
            ret.push_back(pid);
        }
    }
    pclose(proc);
    return ret;
}

bool ProcessesHelper::isProcessFinished(pid_t pid) const
{
    return kill(pid, 0) == -1;
}
