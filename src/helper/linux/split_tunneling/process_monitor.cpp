#include "process_monitor.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <linux/cn_proc.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <poll.h>
#include <sstream>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <spdlog/spdlog.h>

#include "cgroups.h"
#include "../utils.h"

#define SEND_MESSAGE_LEN (NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(enum proc_cn_mcast_op)))
#define RECV_MESSAGE_LEN (NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct proc_event)))
#define SEND_MESSAGE_SIZE (NLMSG_SPACE(SEND_MESSAGE_LEN))
#define RECV_MESSAGE_SIZE (NLMSG_SPACE(RECV_MESSAGE_LEN))
#define BUFSIZE (std::max(std::max(SEND_MESSAGE_SIZE, RECV_MESSAGE_SIZE), 1024UL))

namespace {

// /proc/<pid>/exe is always a fully resolved path, so a stored app path that traverses a symlink
// (Steam's ~/.steam/steam -> ~/.local/share/Steam, usrmerge /bin -> /usr/bin, ...) can never
// compare equal to it as a raw string. Resolve the stored path once, when the app list is set,
// so both sides of the comparison are canonical. Snap entries are exempt: /snap/bin/<name> is a
// symlink to the snap runner ELF itself, and the snap match in compareCmd depends on the stored
// path keeping its /snap/ shape.
std::string canonicalizeAppPath(const std::string &path)
{
    if (path.empty() || path.front() != '/' || path.rfind("/snap/", 0) == 0) {
        return path;
    }
    char *resolved = realpath(path.c_str(), nullptr);
    if (resolved == nullptr) {
        // Not on disk (uninstalled or moved): keep the literal form, same as before.
        return path;
    }
    std::string out(resolved);
    free(resolved);
    return out;
}

// Tokens of the shebang line ("#!/bin/bash -e" -> {"/bin/bash", "-e"}), or nullopt when the
// file is not a script. Only ever called on paths already stat-verified as regular files:
// opening anything else (a FIFO with no writer, for instance) would block the caller
// indefinitely.
std::optional<std::vector<std::string>> readShebangTokens(const std::string &path)
{
    if (path.empty() || path.front() != '/') {
        return std::nullopt;
    }
    std::ifstream f(path);
    std::string line;
    if (!std::getline(f, line)) {
        return std::nullopt;
    }
    if (line.rfind("#!", 0) != 0) {
        return std::nullopt;
    }
    std::istringstream tokens(line.substr(2));
    std::vector<std::string> words;
    std::string word;
    while (tokens >> word) {
        words.push_back(word);
    }
    if (words.empty()) {
        return std::nullopt;
    }
    return words;
}

std::string findExecutableInPath(const std::string &name)
{
    if (name.empty() || name.find('/') != std::string::npos) {
        return std::string();
    }
    const char *pathEnv = getenv("PATH");
    if (pathEnv == nullptr) {
        return std::string();
    }
    std::istringstream dirs(pathEnv);
    std::string dir;
    while (std::getline(dirs, dir, ':')) {
        if (dir.empty()) {
            continue;
        }
        const std::string candidate = dir + "/" + name;
        struct stat st = {};
        if (stat(candidate.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            return candidate;
        }
    }
    return std::string();
}
std::optional<std::vector<std::string>> readArgv(pid_t pid)
{
    std::ifstream f("/proc/" + std::to_string(pid) + "/cmdline", std::ios::binary);
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.empty()) {
        return std::nullopt; // kernel threads and zombies have an empty cmdline
    }
    std::vector<std::string> argv;
    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find('\0', start);
        if (end == std::string::npos) {
            end = data.size();
        }
        if (end > start) {
            argv.emplace_back(data, start, end - start);
        }
        start = end + 1;
    }
    if (argv.empty()) {
        return std::nullopt;
    }
    return argv;
}

} // namespace

void ProcessMonitor::monitorWorker(void *ctx)
{
    char buff[BUFSIZE];

    spdlog::debug("process monitor thread started");
    running_ = true;

    while (running_) {
        struct pollfd pfd;
        pfd.fd = sock_;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 250);
        if (ret < 0) {
            spdlog::error("process monitor poll error {}", ret);
            return;
        } else if (ret == 0) {
            continue;
        }

        ret = read(sock_, &buff, sizeof(buff));
        if (ret <= 0) {
            continue;
        }

        if (testing_) {
            break;
        }

        // Snapshot the rules once per batch: setApps() may replace them on the command thread
        // while this loop runs, and the monitor thread must never iterate a mutating vector.
        std::vector<AppRule> rules;
        {
            std::lock_guard<std::mutex> guard(appsMutex_);
            rules = rules_;
        }

        struct nlmsghdr *nlh = (struct nlmsghdr *)buff;
        while (NLMSG_OK(nlh, ret)) {
            if (nlh->nlmsg_type == NLMSG_NOOP) {
                nlh = NLMSG_NEXT(nlh, ret);
                continue;
            }
            if ((nlh->nlmsg_type == NLMSG_ERROR) || (nlh->nlmsg_type == NLMSG_OVERRUN)) {
                break;
            }
            struct cn_msg *cn_hdr = (struct cn_msg *)NLMSG_DATA(nlh);
            struct proc_event *ev = (struct proc_event *)cn_hdr->data;

            switch (ev->what) {
                case 0x00000001: // PROC_EVENT_FORK:
                    if (compareCmd(ev->event_data.fork.child_pid, rules)) {
                        CGroups::instance().addApp(ev->event_data.fork.child_pid);
                    }
                    break;
                case 0x00000002: // PROC_EVENT_EXEC:
                    if (compareCmd(ev->event_data.exec.process_pid, rules)) {
                        CGroups::instance().addApp(ev->event_data.exec.process_pid);
                    }
                    break;
                case 0x80000000: // PROC_EVENT_EXIT:
                    // Kernel auto-removes the task from its cgroup on exit; no work needed.
                    // (The previous compareCmd-gated removeApp was a no-op either way.)
                    break;
                default:
                    break;
            }

            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }
            nlh = NLMSG_NEXT(nlh, ret);
        }
    }
    running_ = false;
    close(sock_);
    sock_ = -1;
    spdlog::debug("process monitor thread exiting");
}

ProcessMonitor::ProcessMonitor() : isEnabled_(false), thread_(nullptr), sock_(-1), running_(false), functional_(false), testing_(false)
{
    selfTest();
}

ProcessMonitor::~ProcessMonitor()
{
    stopMonitoring();
}

void ProcessMonitor::setApps(const std::vector<std::string> &apps)
{
    // Build the rules before taking the lock: ruleFor does filesystem I/O, and the monitor
    // thread needs the same mutex for its per-batch snapshot — holding it across stat/open
    // calls on a slow path would stall event processing.
    std::vector<AppRule> newRules;
    newRules.reserve(apps.size());
    for (const auto &app : apps) {
        newRules.push_back(ruleFor(app));
    }

    std::vector<std::string> oldApps;
    {
        std::lock_guard<std::mutex> guard(appsMutex_);
        oldApps = apps_;
        apps_ = apps;
        rules_ = std::move(newRules);
    }

    if (isEnabled_) {
        for (auto app : apps) {
            if (std::find(oldApps.begin(), oldApps.end(), app) == oldApps.end()) {
                addApp(app);
            }
        }

        for (auto app : oldApps) {
            if (std::find(apps.begin(), apps.end(), app) == apps.end()) {
                removeApp(app);
            }
        }
    }
}

bool ProcessMonitor::enable()
{
    // Busy, try again later.  Not likely to happen since the helper should start much earlier than the app.
    if (testing_) {
        return false;
    }

    // Tested were run and found that we're not able to get some kernel events.  Fail here.
    if (!functional_) {
        spdlog::error("process monitor not functional");
        return false;
    }

    if (isEnabled_) {
        return true;
    }

    spdlog::debug("process monitor enable");

    if (!startMonitoring()) {
        return false;
    }

    std::vector<std::string> apps;
    {
        std::lock_guard<std::mutex> guard(appsMutex_);
        apps = apps_;
    }
    for (auto app : apps) {
        addApp(app);
    }
    isEnabled_ = true;
    return true;
}

void ProcessMonitor::disable()
{
    if (!isEnabled_) {
        return;
    }

    spdlog::debug("process monitor disable");

    stopMonitoring();

    isEnabled_ = false;
}

ProcessMonitor::AppRule ProcessMonitor::ruleFor(const std::string &app)
{
    // Normalize trailing slashes so directory prefix matching never compares against "dir//file".
    std::string path = app;
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }

    AppRule rule;
    rule.raw = path;
    rule.canonical = canonicalizeAppPath(rule.raw);
    struct stat st = {};
    if (stat(rule.canonical.c_str(), &st) != 0) {
        return rule;
    }
    if (S_ISDIR(st.st_mode)) {
        rule.isDirectory = true;
        rule.canonicalSlash = rule.canonical + "/";
        rule.rawSlash = rule.raw + "/";
    } else if (S_ISREG(st.st_mode)) {
        // Scripts only; anything else (FIFO, socket, device) must never be opened here.
        const std::optional<std::vector<std::string>> shebang = readShebangTokens(rule.canonical);
        if (shebang) {
            std::string interpreter = canonicalizeAppPath((*shebang)[0]);
            // "#!/usr/bin/env bash" execs env, which immediately execs bash; resolve the env
            // argument so the durable interpreter state is what matches (the env moment itself
            // lasts only microseconds).
            if (interpreter.size() >= 4
                && interpreter.compare(interpreter.size() - 4, 4, "/env") == 0
                && shebang->size() >= 2 && (*shebang)[1][0] != '-') {
                const std::string resolved = findExecutableInPath((*shebang)[1]);
                if (!resolved.empty()) {
                    interpreter = canonicalizeAppPath(resolved);
                }
            }
            rule.scriptInterpreter = std::move(interpreter);
        }
    }
    return rule;
}

void ProcessMonitor::addApp(const std::string &exe) {
    spdlog::info("process monitor add app: {}", exe);
    const std::vector<pid_t> pids = expandToDescendants(findPids(exe));
    for (auto pid : pids) {
        CGroups::instance().addApp(pid);
    }
}

void ProcessMonitor::removeApp(const std::string &exe) {
    spdlog::info("process monitor remove app: {}", exe);
    const std::vector<pid_t> pids = expandToDescendants(findPids(exe));
    for (auto pid : pids) {
        CGroups::instance().removeApp(pid);
    }
}

// A matched launcher's already-running children (a game started before its entry was added, or
// before the VPN connected) are not in the cgroup: membership is inherited only on fork, never
// retroactively. When a scan matches a root process, pull its whole living subtree in with it,
// and move it back out on removal, symmetrically.
std::vector<pid_t> ProcessMonitor::expandToDescendants(const std::vector<pid_t> &roots)
{
    if (roots.empty()) {
        return roots;
    }

    std::unordered_map<pid_t, pid_t> ppidOf;
    DIR *dp = opendir("/proc");
    if (dp == NULL) {
        return roots;
    }
    struct dirent *ep;
    while ((ep = readdir(dp))) {
        if (ep->d_type != DT_DIR || ep->d_name[0] < '0' || ep->d_name[0] > '9') {
            continue;
        }
        std::ifstream f(std::string("/proc/") + ep->d_name + "/stat");
        std::string line;
        if (!std::getline(f, line)) {
            continue;
        }
        // pid (comm) state ppid ...: comm may contain spaces or parens, so parse after the
        // last ')'.
        const size_t close = line.rfind(')');
        if (close == std::string::npos) {
            continue;
        }
        std::istringstream fields(line.substr(close + 1));
        std::string state;
        pid_t ppid = 0;
        if (fields >> state >> ppid) {
            ppidOf[std::stoi(ep->d_name)] = ppid;
        }
    }
    closedir(dp);

    std::unordered_set<pid_t> selected(roots.begin(), roots.end());
    for (const auto &entry : ppidOf) {
        pid_t cur = entry.second;
        for (int depth = 0; cur > 0 && depth < 128; ++depth) {
            if (selected.count(cur)) {
                selected.insert(entry.first);
                break;
            }
            const auto it = ppidOf.find(cur);
            if (it == ppidOf.end()) {
                break;
            }
            cur = it->second;
        }
    }
    return std::vector<pid_t>(selected.begin(), selected.end());
}

std::vector<pid_t> ProcessMonitor::findPids(const std::string &exe)
{
    std::vector<pid_t> pids;
    const AppRule rule = ruleFor(exe); // classify the entry once, not once per /proc entry

    DIR *dp = NULL;
    struct dirent *ep;

    dp = opendir("/proc");
    if (dp == NULL) {
        spdlog::error("process monitor could not open /proc filesystem");
        return pids;
    }

    while ((ep = readdir(dp))) {
        // numeric directories are pids in /proc
        if (ep->d_type == DT_DIR && ep->d_name[0] >= '0' && ep->d_name[0] <= '9') {
            if (compareCmd(std::stoi(ep->d_name), {rule})) {
                pids.push_back(std::stoi(ep->d_name));
            }
        }
    }
    closedir(dp);

    return pids;
}

bool ProcessMonitor::compareCmd(pid_t pid, const std::vector<AppRule> &rules) {
    std::string cmd = getCmdByPid(pid);
    std::optional<std::vector<std::string>> argv;
    bool argvRead = false;
    std::optional<std::string> flatpakId;
    bool flatpakIdComputed = false;

    for (const AppRule &rule : rules) {
        if (!cmd.empty() && (cmd == rule.raw || cmd == rule.canonical)) {
            return true;
        }

        // Flatpak app ID match: stored value looks like reverse-DNS (no slashes, has a dot).
        // Resolve the running process's Flatpak app ID lazily via cgroup, only when we actually
        // have an app-ID-shaped rule to match against.
        if (!rule.raw.empty() && rule.raw.find('/') == std::string::npos && rule.raw.find('.') != std::string::npos) {
            if (!flatpakIdComputed) {
                flatpakId = getFlatpakAppIdByPid(pid);
                flatpakIdComputed = true;
            }
            if (flatpakId && *flatpakId == rule.raw) {
                return true;
            }
        }

        // handle snap
        int idx = rule.raw.find("/snap/");
        if (idx != std::string::npos) {
            std::string prefix = rule.raw.substr(0, idx + 6);
            std::string suffix = rule.raw.substr(rule.raw.rfind("/"));
            if (cmd.rfind(prefix, 0) == 0 && cmd.find(suffix, cmd.size() - suffix.length()) == cmd.size() - suffix.length()) {
                return true;
            }
        }

        // Directory entries (e.g. a Steam game's install folder, resolved from its library
        // manifest): every executable launched from within the tree belongs to the entry,
        // whatever each binary is named (launcher, helper, the game itself).
        if (rule.isDirectory && !cmd.empty()
            && (cmd.rfind(rule.canonicalSlash, 0) == 0 || cmd.rfind(rule.rawSlash, 0) == 0)) {
            return true;
        }

        // Script-wrapped launchers (e.g. Steam's /usr/bin/steam, a shell script that execs the
        // real client ELF from the Steam root). /proc/<pid>/exe is never a script path — while
        // the interpreter runs the script, exe is the interpreter and the script path appears
        // in argv — so a script entry matches only at that moment, and only when the process's
        // exe IS the interpreter the script's shebang names and an argv token equals the stored
        // path. Both halves are required: the interpreter check keeps unrelated programs that
        // merely mention the path as a data argument (grep, editors) out, and matching on the
        // full path instead of a basename keeps same-named binaries elsewhere on the system
        // out. The single match point is sufficient: cgroup membership survives the exec of the
        // wrapped binary, and every descendant of the launcher (the games it spawns) inherits
        // it on fork. One consequence is deliberate: a launcher that was already exec'd before
        // monitoring began cannot be identified anymore and waits for its next launch.
        if (!rule.scriptInterpreter.empty() && cmd == rule.scriptInterpreter) {
            if (!argvRead) {
                argv = readArgv(pid);
                argvRead = true;
            }
            if (argv) {
                for (const std::string &token : *argv) {
                    if (token == rule.raw || token == rule.canonical) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

std::optional<std::string> ProcessMonitor::getFlatpakAppIdByPid(pid_t pid)
{
    std::ifstream f("/proc/" + std::to_string(pid) + "/cgroup");
    if (!f.is_open()) {
        return std::nullopt;
    }

    const std::string marker = "app-flatpak-";
    const std::string scopeSuffix = ".scope";

    std::string line;
    while (std::getline(f, line)) {
        size_t mpos = line.find(marker);
        if (mpos == std::string::npos) continue;

        size_t spos = line.find(scopeSuffix, mpos);
        if (spos == std::string::npos) continue;

        // Token is "<app-id>-<launcher-pid>" between marker and ".scope".
        size_t tokStart = mpos + marker.size();
        std::string token = line.substr(tokStart, spos - tokStart);

        size_t dashPos = token.rfind('-');
        if (dashPos == std::string::npos || dashPos == 0) continue;

        // Trailing component must be all digits (the launcher PID).
        bool allDigits = (dashPos + 1 < token.size());
        for (size_t i = dashPos + 1; i < token.size() && allDigits; i++) {
            if (token[i] < '0' || token[i] > '9') allDigits = false;
        }
        if (!allDigits) continue;

        return token.substr(0, dashPos);
    }
    return std::nullopt;
}

std::string ProcessMonitor::getCmdByPid(pid_t pid)
{
    char buf[PATH_MAX];

    memset(buf, 0, PATH_MAX);

    int ret = readlink((std::string("/proc/") + std::to_string(pid) + "/exe").c_str(), buf, PATH_MAX - 1);
    if (ret < 0) {
        return std::string();
    }
    return buf;
}

bool ProcessMonitor::startMonitoring()
{
    if (thread_) {
        // already monitoring, do nothing
        return true;
    }

    if (!prepareMonitoring()) {
        spdlog::error("Failed to prepare monitoring");
        return false;
    }

    try {
        thread_ = new std::thread(&ProcessMonitor::monitorWorker, this, &sock_);
    } catch (std::exception &e) {
        spdlog::error("process monitor caught exception starting thread");
    }

    return true;
}

bool ProcessMonitor::prepareMonitoring()
{
    int ret = 0;
    struct sockaddr_nl addr;
    char buf[BUFSIZE];

    sock_ = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (sock_ == -1) {
        spdlog::error("Could not open netlink socket");
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = CN_IDX_PROC;
    addr.nl_pid = getpid();

    ret = bind(sock_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        spdlog::error("Could not bind netlink socket");
        close(sock_);
        sock_ = -1;
        return false;
    }

    // Request for events
    struct nlmsghdr *nl_hdr = (struct nlmsghdr *)buf;
    struct cn_msg *cn_hdr = (struct cn_msg *)NLMSG_DATA(nl_hdr);
    enum proc_cn_mcast_op *mcop_msg = (enum proc_cn_mcast_op *)&cn_hdr->data[0];

    memset(buf, 0, sizeof(buf));
    *mcop_msg = PROC_CN_MCAST_LISTEN;

    nl_hdr->nlmsg_len = SEND_MESSAGE_LEN;
    nl_hdr->nlmsg_pid = getpid();
    nl_hdr->nlmsg_type = NLMSG_DONE;
    nl_hdr->nlmsg_flags = 0;
    nl_hdr->nlmsg_seq = 0;

    cn_hdr->id.idx = CN_IDX_PROC;
    cn_hdr->id.val = CN_VAL_PROC;
    cn_hdr->len = sizeof(enum proc_cn_mcast_op);
    cn_hdr->seq = 0;
    cn_hdr->ack = 0;

    ret = send(sock_, nl_hdr, nl_hdr->nlmsg_len, 0); // NOLINT
    if (ret == -1) {
        spdlog::error("Could not request events");
        close(sock_);
        sock_ = -1;
        return false;
    }
    return true;
}

void ProcessMonitor::stopMonitoring()
{
    if (sock_ != -1) {
        running_ = false;
    }

    if (thread_) {
        if (thread_->joinable()) {
            thread_->join();
        }
        delete thread_;
        thread_ = nullptr;
    }
}

void ProcessMonitor::selfTest()
{
    testing_ = true;

    if (!startMonitoring()) {
        functional_ = false;
        return;
    }

    // Starts a process to test the process monitor
    int ret = system("echo");
    UNUSED(ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Thread should no longer be running
    functional_ = !running_;

    // Cleanup
    stopMonitoring();

    spdlog::debug("process monitor self-test {}", (functional_ ? "successful" : "unsuccessful"));
    testing_ = false;
}
