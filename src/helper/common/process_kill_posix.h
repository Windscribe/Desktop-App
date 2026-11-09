#pragma once

#include <chrono>
#include <string>

namespace ProcessKill {

// pkill only *sends* the signal; it does not wait for the process to exit. A still-dying instance
// keeps holding what a freshly spawned one needs (a listening port, a socket path), so this sends
// SIGTERM to every process whose command line contains exeName, waits for them to actually go away,
// and escalates to SIGKILL if they don't. Returns false if one is still present after SIGKILL.
// gracePeriod is monotonic elapsed time: it does not grow with how long each liveness check takes.
// A check that returns after the deadline is repeated once, because a slow pkill can report a
// process it sampled before that process had exited.
bool terminateAndWait(const std::string &exeName,
                      std::chrono::milliseconds gracePeriod = std::chrono::seconds(5));

} // namespace ProcessKill
