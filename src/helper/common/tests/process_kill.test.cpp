#include "../process_kill_posix.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

std::string g_selfPath;

bool targetVisible(const std::string &name)
{
    return !ProcessKill::findByExecutableName(name).empty();
}

std::string makeTargetExecutable(const std::string &name)
{
    std::string dirTemplate = (std::filesystem::path(g_selfPath).parent_path() / "wsprocesskilltest.XXXXXX").string();
    const char *dir = mkdtemp(dirTemplate.data());
    if (!dir) {
        return {};
    }
    const std::string path = std::string(dir) + "/" + name;
    std::error_code ec;
    std::filesystem::copy_file(g_selfPath, path, ec);
    if (ec) {
        std::filesystem::remove_all(dir, ec);
        return {};
    }
    return path;
}

void removeTargetExecutable(const std::string &path)
{
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(path).parent_path(), ec);
}

// A missing gracePeriod calls terminateAndWait with its default, the way the helper's callers do.
bool check(const char *label, bool ignoreSigterm, std::optional<std::chrono::milliseconds> gracePeriod,
           double minSeconds, double maxSeconds)
{
    const std::string name = "wsprocesskilltest" + std::to_string(getpid()) + label;
    const std::string exePath = makeTargetExecutable(name);
    if (exePath.empty()) {
        std::fprintf(stderr, "%s: could not create the target executable\n", label);
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        std::fprintf(stderr, "%s: fork failed\n", label);
        return false;
    }
    if (pid == 0) {
        if (ignoreSigterm) {
            signal(SIGTERM, SIG_IGN);
        }
        // Unique executable name so only this process matches.
        execl(exePath.c_str(), exePath.c_str(), "--sleep", static_cast<char *>(nullptr));
        _exit(127);
    }
    // Reap immediately, as init would for the real daemons, so a dead target does not linger as a
    // zombie that is still listed. The collected status is how the test tells SIGTERM from
    // the SIGKILL escalation; the time limit only stops a wait that never ends.
    std::atomic<int> status{-1};
    std::thread reaper([pid, &status] {
        int local = 0;
        if (waitpid(pid, &local, 0) == pid) {
            status.store(local, std::memory_order_relaxed);
        }
    });
    const auto finish = [&] {
        int local = 0;
        const pid_t got = waitpid(pid, &local, WNOHANG);
        if (got == 0) {
            kill(pid, SIGKILL);
        } else if (got == pid) {
            status.store(local, std::memory_order_relaxed);
        }
        reaper.join();
        removeTargetExecutable(exePath);
    };

    // Time for exec to replace the executable, measured on the clock so slow scans don't stretch it.
    const auto appearDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool visible = targetVisible(name);
    while (!visible && std::chrono::steady_clock::now() < appearDeadline) {
        usleep(50 * 1000);
        visible = targetVisible(name);
    }
    if (!visible) {
        std::fprintf(stderr, "%s: target did not appear\n", label);
        finish();
        return false;
    }

    const auto start = std::chrono::steady_clock::now();
    const bool gone = gracePeriod ? ProcessKill::terminateAndWait(name, *gracePeriod)
                                  : ProcessKill::terminateAndWait(name);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    finish();
    const int collected = status.load(std::memory_order_relaxed);
    const int termSig = WIFSIGNALED(collected) ? WTERMSIG(collected) : 0;
    const int expectedSignal = ignoreSigterm ? SIGKILL : SIGTERM;
    if (!gone || termSig != expectedSignal || seconds < minSeconds || seconds > maxSeconds) {
        std::fprintf(stderr, "%s: gone=%d signal=%d after %.2fs, expected signal %d and %.1f-%.1fs\n",
                     label, gone, termSig, seconds, expectedSignal, minSeconds, maxSeconds);
        return false;
    }
    return true;
}

// A root helper must not signal a process that only mentions the name in its arguments.
bool checkArgumentOnlyMatchIsLeftAlone()
{
    const std::string name = "wsprocesskilltest" + std::to_string(getpid()) + "argonly";
    const std::string exeName = name + "exe";
    const std::string exePath = makeTargetExecutable(exeName);
    if (exePath.empty()) {
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        removeTargetExecutable(exePath);
        return false;
    }
    if (pid == 0) {
        execl(exePath.c_str(), name.c_str(), "--sleep", static_cast<char *>(nullptr));
        _exit(127);
    }
    const auto execDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    auto execed = [pid, &exeName] {
        const auto found = ProcessKill::findByExecutableName(exeName);
        return std::find(found.begin(), found.end(), pid) != found.end();
    };
    while (!execed() && std::chrono::steady_clock::now() < execDeadline) {
        usleep(10 * 1000);
    }
    const bool started = execed();
    const bool found = targetVisible(name);
    const bool gone = ProcessKill::terminateAndWait(name, std::chrono::milliseconds(100));
    int status = 0;
    const bool stillRunning = waitpid(pid, &status, WNOHANG) == 0;
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    removeTargetExecutable(exePath);
    if (!started || found || !gone || !stillRunning) {
        std::fprintf(stderr, "argonly: started=%d found=%d gone=%d stillRunning=%d\n", started, found, gone, stillRunning);
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--sleep") {
        for (;;) {
            pause();
        }
    }
    char selfPath[PATH_MAX];
    if (!realpath(argv[0], selfPath)) {
        std::fprintf(stderr, "could not resolve the test executable path\n");
        return 1;
    }
    g_selfPath = selfPath;
    using std::chrono::milliseconds;
    // Honouring SIGTERM must end on that signal, and as soon as the target is gone rather than when the
    // grace period runs out. The grace period is twice the upper bound, so a wait that always sleeps it
    // out fails on the clock, while a slow process scan still has plenty of room before that bound.
    bool ok = check("term", false, milliseconds(20000), 0.0, 10.0);
    // Ignoring SIGTERM must survive until the grace period has elapsed and then die on SIGKILL.
    // The upper bound only stops a wait that is not bounded by the clock.
    ok = check("kill", true, milliseconds(500), 0.5, 15.0) && ok;
    // The default grace period is what the helper's callers get: a dying ctrld or OpenVPN needs those
    // seconds to release its port or socket before SIGKILL. The deadline is monotonic, so it is a hard
    // lower bound.
    ok = check("default", true, std::nullopt, 5.0, 20.0) && ok;
    ok = ProcessKill::terminateAndWait("wsprocesskilltestabsent") && ok;
    ok = checkArgumentOnlyMatchIsLeftAlone() && ok;
    return ok ? 0 : 1;
}
