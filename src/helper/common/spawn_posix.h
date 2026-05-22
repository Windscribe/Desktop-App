#pragma once

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
bool spawnDetached(const std::string &exePath,
                   const std::vector<std::string> &args,
                   const Options &opts = {});

} // namespace Spawn
