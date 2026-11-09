#include "process_kill_posix.h"

#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <spdlog/spdlog.h>

namespace Utils {
int executeCommand(const std::string &cmd,
                   const std::vector<std::string> &args = std::vector<std::string>(),
                   std::string *pOutputStr = nullptr, bool appendFromStdErr = true);
}

namespace ProcessKill {

bool terminateAndWait(const std::string &exeName, std::chrono::milliseconds gracePeriod)
{
    if (exeName.empty()) {
        return true;
    }
    // executeCommand runs through a shell whose own command line contains the pattern; the bracket
    // expression still matches the target but not that shell, which would otherwise never go away.
    const std::string pattern = "[" + exeName.substr(0, 1) + "]" + exeName.substr(1);

    // Bounded by a deadline rather than an iteration count: each pkill -0 spawns a shell and can
    // take a noticeable fraction of a second on a loaded machine. pkill reports a match it saw
    // while scanning, so a scan that outlives the process is confirmed once more before the wait
    // is treated as expired.
    auto stillAlive = [&pattern] {
        return Utils::executeCommand("pkill", {"-0", "-f", pattern}) == 0;
    };
    auto waitGone = [&stillAlive](std::chrono::milliseconds timeout) {
        using Clock = std::chrono::steady_clock;
        const auto deadline = Clock::now() + timeout;
        for (;;) {
            if (!stillAlive()) {
                return true;
            }
            const auto remaining = deadline - Clock::now();
            if (remaining <= Clock::duration::zero()) {
                return !stillAlive();
            }
            const auto pause = std::min<Clock::duration>(remaining, std::chrono::milliseconds(50));
            usleep(static_cast<useconds_t>(std::chrono::duration_cast<std::chrono::microseconds>(pause).count()));
        }
    };

    Utils::executeCommand("pkill", {"-f", pattern});
    if (waitGone(gracePeriod)) {
        return true;
    }

    spdlog::warn("{} did not exit after SIGTERM; sending SIGKILL", exeName);
    Utils::executeCommand("pkill", {"-KILL", "-f", pattern});
    if (waitGone(std::chrono::seconds(1))) {   // time to confirm reap
        return true;
    }
    spdlog::error("{} still present after SIGKILL", exeName);
    return false;
}

} // namespace ProcessKill
