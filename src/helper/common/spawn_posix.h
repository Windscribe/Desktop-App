#pragma once

#include <sys/types.h>

#include <string>
#include <utility>
#include <vector>

namespace Spawn {

struct Options {
    std::string cwd;
    std::string runAsUser;
    std::vector<std::pair<std::string, std::string>> extraEnv;
};

// Fork+exec a daemon detached from the helper. The child calls setsid(), optionally
// chdir()s, optionally drops privileges to runAsUser via getpwnam+initgroups+setgid+setuid,
// applies extraEnv (child-only — does not leak into the helper), redirects stdin to
// /dev/null, and execs exePath with argv = {exePath, args...}. Inherits parent's
// stdout/stderr so daemon output flows to launchd/journald.
//
// exePath must already be canonicalized (use Utils::resolveExePath). args are passed as
// argv elements directly — no shell, no quoting.
//
// Returns true only if fork() and execv() both succeeded. exec failures are detected via
// a self-pipe and surfaced synchronously to the caller.
//
// If outDaemonPid is non-null, it receives the pid of the detached daemon so the caller can
// probe its liveness with kill(pid, 0) (e.g. to bail out of a wait loop when the daemon dies
// early). It is set to 0 when the pid could not be captured (spawn still succeeds); callers must
// treat 0 as "unknown" and fall back to their timeout. Only the posix_spawn (no-privilege-drop)
// path reports a pid; the double-fork privilege-drop path always reports 0.
bool spawnDetached(const std::string &exePath,
                   const std::vector<std::string> &args,
                   const Options &opts = {},
                   pid_t *outDaemonPid = nullptr);

} // namespace Spawn
