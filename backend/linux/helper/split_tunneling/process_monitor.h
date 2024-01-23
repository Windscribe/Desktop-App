#pragma once

#include <string>
#include <thread>
#include <vector>

class ProcessMonitor
{
public:
    static ProcessMonitor& instance()
    {
        static ProcessMonitor pm;
        return pm;
    }

    void setApps(const std::vector<std::string> &apps);
    bool enable();
    void disable();

private:
    bool isEnabled_;
    std::vector<std::string> apps_;
    std::thread *thread_;
    int sock_;

    ProcessMonitor();
    ~ProcessMonitor();

    void addApp(const std::string &exe);
    void removeApp(const std::string &exe);
    std::vector<pid_t> findPids(const std::string &exe);
    std::string getCmdByPid(pid_t pid);

    bool prepareMonitoring();
    bool startMonitoring();
    void stopMonitoring();
    void monitorWorker(void *ctx);
    bool compareCmd(pid_t pid, const std::vector<std::string> &exes);
};

