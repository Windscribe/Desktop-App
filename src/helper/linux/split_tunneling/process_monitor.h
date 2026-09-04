#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <sys/types.h>

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
    // One client-supplied app entry preprocessed into the forms used for matching.
    struct AppRule
    {
        std::string raw;             // as supplied, minus trailing slashes (snap prefix/suffix and Flatpak ID matching use this)
        std::string canonical;       // realpath()-resolved; the same form /proc/<pid>/exe reports
        std::string canonicalSlash;  // canonical + "/"; precomputed for directory prefix matching
        std::string rawSlash;        // raw + "/"; same, for the unresolved spelling
        std::string scriptInterpreter;  // canonical shebang interpreter; non-empty => script-wrapped launcher
        bool isDirectory = false;    // the stored path is a directory (a whole app/game install tree)
    };

    bool isEnabled_;
    std::vector<std::string> apps_;  // raw entries in client form (add/remove diff logic)
    std::vector<AppRule> rules_;     // matching form of apps_, rebuilt together with apps_
    std::mutex appsMutex_;           // guards apps_/rules_ against the monitor thread

    std::thread *thread_;
    int sock_;
    bool running_;

    bool functional_;
    bool testing_;

    ProcessMonitor();
    ~ProcessMonitor();

    static AppRule ruleFor(const std::string &app);
    static std::vector<pid_t> expandToDescendants(const std::vector<pid_t> &roots);

    void addApp(const std::string &exe);
    void removeApp(const std::string &exe);
    std::vector<pid_t> findPids(const std::string &exe);
    std::string getCmdByPid(pid_t pid);
    std::optional<std::string> getFlatpakAppIdByPid(pid_t pid);

    void selfTest();
    bool prepareMonitoring();
    bool startMonitoring();
    void stopMonitoring();
    void monitorWorker(void *ctx);
    bool compareCmd(pid_t pid, const std::vector<AppRule> &rules);
};
