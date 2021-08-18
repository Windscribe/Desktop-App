#include "processes_helper.h"
#include <signal.h>
#include <libproc.h>

ProcessesHelper::ProcessesHelper()
{
    int numberOfProcesses = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    pid_t pids[numberOfProcesses];
    bzero(pids, sizeof(pids));
    
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids, (int)sizeof(pids));
    int n_proc = bytes / sizeof(pids[0]);
    processes_.resize(n_proc);
    for (int i = 0; i < n_proc; i++)
    {
        struct proc_bsdinfo proc;
        int st = proc_pidinfo(pids[i], PROC_PIDTBSDINFO, 0,
                             &proc, PROC_PIDTBSDINFO_SIZE);
        if (st == PROC_PIDTBSDINFO_SIZE)
        {
            processes_.push_back(std::make_pair(proc.pbi_name, pids[i]));
        }
    }
}

std::vector<pid_t> ProcessesHelper::getPidsByProcessname(const char *name)
{
    std::vector<pid_t> ret;
    for (auto it : processes_)
    {
        if (strcmp(it.first.c_str(), name) == 0)
        {
            ret.push_back(it.second);
        }
    }
    return ret;
}

bool ProcessesHelper::isProcessFinished(pid_t pid) const
{
    return kill(pid, 0) == -1;
}
