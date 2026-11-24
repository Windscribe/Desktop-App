#pragma once

#include <sys/types.h>

#include <chrono>
#include <string>
#include <vector>

namespace ProcessKill {

std::vector<pid_t> findByExecutableName(const std::string &exeName);
bool isAnyExecutablePathContaining(const std::string &fragment);

void terminate(const std::string &exeName);

// Sends SIGTERM and waits for the processes to exit, escalating to SIGKILL after gracePeriod. A still-dying instance
// keeps holding what a freshly spawned one needs (a port, a socket path). Returns false if one survives SIGKILL.
bool terminateAndWait(const std::string &exeName,
                      std::chrono::milliseconds gracePeriod = std::chrono::seconds(5));

} // namespace ProcessKill
