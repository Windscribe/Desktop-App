#include "../process_kill_posix.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Mirrors the helper's executeCommand: the command line goes through a shell, so the shell's own
// command line contains the pkill pattern, which is what the bracket pattern has to tolerate.
namespace Utils {
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string * = nullptr, bool = true)
{
    std::string line = cmd;
    for (const auto &arg : args) {
        line += " '" + arg + "'";
    }
    line += " >/dev/null 2>&1";
    const int status = std::system(line.c_str());
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
}

namespace {

// Same bracket pattern as ProcessKill: the shell that runs pkill contains the pattern, and without
// the brackets pkill -0 would match that shell and look like the target is already there.
bool targetVisible(const std::string &name)
{
    const std::string pattern = "[" + name.substr(0, 1) + "]" + name.substr(1);
    return Utils::executeCommand("pkill", {"-0", "-f", pattern}) == 0;
}

// A missing gracePeriod calls terminateAndWait with its default, the way the helper's callers do.
bool check(const char *label, bool ignoreSigterm, std::optional<std::chrono::milliseconds> gracePeriod,
           double minSeconds, double maxSeconds)
{
    const std::string name = "wsprocesskilltest" + std::to_string(getpid()) + label;
    const pid_t pid = fork();
    if (pid < 0) {
        std::fprintf(stderr, "%s: fork failed\n", label);
        return false;
    }
    if (pid == 0) {
        if (ignoreSigterm) {
            signal(SIGTERM, SIG_IGN);
        }
        // Unique argv[0] so pkill -f matches only this process.
        execl("/bin/sleep", name.c_str(), "60", static_cast<char *>(nullptr));
        _exit(127);
    }
    // Reap immediately, as init would for the real daemons, so a dead target does not linger as a
    // zombie that pkill -0 still reports. The collected status is how the test tells SIGTERM from
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
    };

    // Time for exec to replace argv, measured on the clock so slow pkill runs don't stretch it.
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

} // namespace

int main()
{
    using std::chrono::milliseconds;
    // Honouring SIGTERM must end on that signal, and as soon as the target is gone rather than when the
    // grace period runs out. The grace period is twice the upper bound, so a wait that always sleeps it
    // out fails on the clock, while a slow pkill still has plenty of room before that bound.
    bool ok = check("term", false, milliseconds(20000), 0.0, 10.0);
    // Ignoring SIGTERM must survive until the grace period has elapsed and then die on SIGKILL.
    // The upper bound only stops a wait that is not bounded by the clock.
    ok = check("kill", true, milliseconds(500), 0.5, 15.0) && ok;
    // The default grace period is what the helper's callers get: a dying ctrld or OpenVPN needs those
    // seconds to release its port or socket before SIGKILL. The deadline is monotonic, so it is a hard
    // lower bound.
    ok = check("default", true, std::nullopt, 5.0, 20.0) && ok;
    ok = ProcessKill::terminateAndWait("wsprocesskilltestabsent") && ok;
    return ok ? 0 : 1;
}
