#include "process_kill_posix.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include "utils/log/debug_trace.h"

#if defined(__APPLE__)
#include <libproc.h>
#include <sys/proc.h>
#else
#include <dirent.h>
#include <fstream>
#include <limits.h>
#include <sstream>
#endif

namespace ProcessKill {

namespace {

template<typename Visit>
void forEachProcess(Visit visit)
{
    const pid_t self = getpid();
#if defined(__APPLE__)
    const int estimate = proc_listallpids(nullptr, 0);
    if (estimate <= 0) {
        return;
    }
    std::vector<pid_t> pids(static_cast<size_t>(estimate) + 64);
    const int count = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
    for (int i = 0; i < count; ++i) {
        const pid_t pid = pids[static_cast<size_t>(i)];
        if (pid <= 0 || pid == self) {
            continue;
        }
        char path[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pid, path, sizeof(path)) > 0) {
            visit(pid, std::string(path));
            continue;
        }
        struct proc_bsdinfo info;
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) == static_cast<int>(sizeof(info)) &&
            info.pbi_status != SZOMB && info.pbi_name[0] != '\0') {
            visit(pid, std::string(info.pbi_name, strnlen(info.pbi_name, sizeof(info.pbi_name))));
        }
    }
#else
    DIR *dir = opendir("/proc");
    if (!dir) {
        return;
    }
    while (const struct dirent *entry = readdir(dir)) {
        char *end = nullptr;
        const long value = strtol(entry->d_name, &end, 10);
        if (end == entry->d_name || *end != '\0' || value <= 0 || static_cast<pid_t>(value) == self) {
            continue;
        }
        const std::string link = std::string("/proc/") + entry->d_name + "/exe";
        char path[PATH_MAX];
        const ssize_t len = readlink(link.c_str(), path, sizeof(path) - 1);
        if (len <= 0) {
            continue;
        }
        std::string exePath(path, static_cast<size_t>(len));
        const std::string deleted = " (deleted)";
        if (exePath.size() > deleted.size() && exePath.compare(exePath.size() - deleted.size(), deleted.size(), deleted) == 0) {
            exePath.resize(exePath.size() - deleted.size());
        }
        visit(static_cast<pid_t>(value), exePath);
    }
    closedir(dir);
#endif
}

std::string fileName(const std::string &path)
{
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

void signalAll(const std::vector<pid_t> &pids, int sig)
{
    for (const pid_t pid : pids) {
        if (kill(pid, sig) != 0 && errno != ESRCH) {
            spdlog::warn("Could not signal process {}: errno {}", pid, errno);
        }
    }
}

// A zombie, or a new process that reused the pid, must not count as the process we signalled.
bool liveStartTime(pid_t pid, uint64_t &start)
{
#if defined(__APPLE__)
    struct proc_bsdinfo info;
    if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) != static_cast<int>(sizeof(info)) || info.pbi_status == SZOMB) {
        return false;
    }
    start = info.pbi_start_tvsec * 1000000ULL + info.pbi_start_tvusec;
    return true;
#else
    std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
    std::string stat;
    std::getline(file, stat);
    const auto paren = stat.rfind(')');
    if (paren == std::string::npos) {
        return false;
    }
    std::istringstream fields(stat.substr(paren + 1));
    std::string state;
    fields >> state;
    if (state == "Z" || state == "X") {
        return false;
    }
    std::string skipped;
    for (int field = 4; field < 22; ++field) {
        fields >> skipped;
    }
    fields >> start;
    return static_cast<bool>(fields);
#endif
}

struct Target
{
    pid_t pid;
    uint64_t start;
};

void addLiveTargets(const std::vector<pid_t> &pids, std::vector<Target> &targets)
{
    for (const pid_t pid : pids) {
        uint64_t start = 0;
        const bool known = std::any_of(targets.begin(), targets.end(), [pid](const Target &t) { return t.pid == pid; });
        if (!known && liveStartTime(pid, start)) {
            targets.push_back({pid, start});
        }
    }
}

bool isAlive(const Target &target)
{
    uint64_t start = 0;
    return liveStartTime(target.pid, start) && start == target.start;
}

void signalTargets(const std::vector<Target> &targets, int sig)
{
    std::vector<pid_t> pids;
    for (const Target &target : targets) {
        pids.push_back(target.pid);
    }
    signalAll(pids, sig);
}

} // namespace

std::vector<pid_t> findByExecutableName(const std::string &exeName)
{
    std::vector<pid_t> result;
    if (exeName.empty()) {
        return result;
    }
    forEachProcess([&](pid_t pid, const std::string &path) {
        if (fileName(path) == exeName) {
            result.push_back(pid);
        }
    });
    return result;
}

bool isAnyExecutablePathContaining(const std::string &fragment)
{
    bool found = false;
    if (fragment.empty()) {
        return found;
    }
    forEachProcess([&](pid_t, const std::string &path) {
        found = found || path.find(fragment) != std::string::npos;
    });
    return found;
}

void terminate(const std::string &exeName)
{
    signalAll(findByExecutableName(exeName), SIGTERM);
}

bool terminateAndWait(const std::string &exeName, std::chrono::milliseconds gracePeriod)
{
    WS_DEBUG_TRACE_SCOPE("ProcessKill::terminateAndWait");
    std::vector<Target> targets;
    addLiveTargets(findByExecutableName(exeName), targets);
    if (targets.empty()) {
        return true;
    }

    auto waitGone = [&targets](std::chrono::milliseconds timeout) {
        using Clock = std::chrono::steady_clock;
        const auto deadline = Clock::now() + timeout;
        for (;;) {
            targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target &t) { return !isAlive(t); }),
                          targets.end());
            if (targets.empty()) {
                return true;
            }
            if (Clock::now() >= deadline) {
                return false;
            }
            usleep(5 * 1000);
        }
    };

    signalTargets(targets, SIGTERM);
    if (waitGone(gracePeriod)) {
        return true;
    }

    spdlog::warn("{} did not exit after SIGTERM; sending SIGKILL", exeName);
    targets.erase(std::remove_if(targets.begin(), targets.end(), [](const Target &t) { return !isAlive(t); }), targets.end());
    addLiveTargets(findByExecutableName(exeName), targets);
    signalTargets(targets, SIGKILL);
    if (waitGone(std::chrono::seconds(1))) {
        return true;
    }
    spdlog::error("{} still present after SIGKILL", exeName);
    return false;
}

} // namespace ProcessKill
