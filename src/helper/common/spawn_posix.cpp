#include "spawn_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "io_posix.h"

extern char **environ;

namespace Spawn {

namespace {

// Build envp in the parent. extraEnv overrides existing entries with the same KEY,
// otherwise the helper's environment is inherited.
std::vector<std::string> buildEnvStorage(const std::vector<std::pair<std::string, std::string>> &extraEnv)
{
    std::vector<std::string> envStorage;
    for (char **e = environ; e && *e; ++e) {
        std::string entry(*e);
        bool overridden = false;
        const size_t eq = entry.find('=');
        if (eq != std::string::npos) {
            const std::string key = entry.substr(0, eq);
            for (const auto &kv : extraEnv) {
                if (kv.first == key) {
                    overridden = true;
                    break;
                }
            }
        }
        if (!overridden) {
            envStorage.push_back(std::move(entry));
        }
    }
    for (const auto &kv : extraEnv) {
        envStorage.push_back(kv.first + "=" + kv.second);
    }
    return envStorage;
}

// Daemon launcher: posix_spawn /bin/sh as a shell-wrapper that forks the daemon,
// backgrounds it, and exits. The daemon ends up reparented to init/launchd, so the
// helper has no zombie to reap. The shell itself is the helper's brief child and we
// waitpid it inline.
//
// Why not posix_spawn the daemon directly? Because that makes the daemon the helper's
// long-running child. Combined with `SIGCHLD = SIG_IGN | SA_NOCLDWAIT` (which we used to
// rely on for auto-reaping), `waitpid(child_pid, 0)` in pstream's executeCommand blocks
// until *all* children terminate. Removing SA_NOCLDWAIT instead means daemon zombies
// accumulate. The shell-wrapper restores the original master semantics.
//
// Why not fork() ourselves? fork() from a libxpc/libdispatch program corrupts atfork
// state and any subsequent fork() (e.g. pstream's) hangs.
//
// Shell injection: the script is the fixed string `"$@" &` — no caller data is
// interpolated into it. The daemon path and args are passed as separate argv elements
// which become positional `$1`, `$2`, ...; quoting `"$@"` preserves them as-is.
bool spawnViaPosixSpawn(const std::string &exePath,
                        const std::vector<std::string> &args,
                        const std::vector<const char *> &envp,
                        const std::string &cwd,
                        pid_t *outDaemonPid)
{
    if (outDaemonPid) {
        *outDaemonPid = 0;
    }

    // When the caller wants the daemon's pid, hand the shell a write pipe on fd 3 and have it print
    // `$!` (the backgrounded daemon's pid) there. `3>&-` closes fd 3 in the daemon itself so the pipe
    // is not leaked into it and the parent sees EOF once the shell exits. Both pipe ends are CLOEXEC;
    // only the explicit fd-3 dup2 below survives into the shell.
    int pidPipe[2] = {-1, -1};
    if (outDaemonPid) {
#if defined(__linux__)
        if (pipe2(pidPipe, O_CLOEXEC) != 0) {
            spdlog::warn("spawnDetached: pid pipe2() failed: {}; daemon pid will be unavailable", IO::strerror(errno));
            pidPipe[0] = pidPipe[1] = -1;
        }
#else
        if (pipe(pidPipe) != 0) {
            spdlog::warn("spawnDetached: pid pipe() failed: {}; daemon pid will be unavailable", IO::strerror(errno));
            pidPipe[0] = pidPipe[1] = -1;
        } else {
            fcntl(pidPipe[0], F_SETFD, FD_CLOEXEC);
            fcntl(pidPipe[1], F_SETFD, FD_CLOEXEC);
        }
#endif
    }
    const bool capturePid = (pidPipe[1] >= 0);
    auto closePidPipe = [&]() {
        if (pidPipe[0] >= 0) { close(pidPipe[0]); pidPipe[0] = -1; }
        if (pidPipe[1] >= 0) { close(pidPipe[1]); pidPipe[1] = -1; }
    };

    // posix_spawn* APIs return an errno value directly (they do not set the global errno).
    posix_spawnattr_t attr;
    int rc = posix_spawnattr_init(&attr);
    if (rc != 0) {
        spdlog::error("spawnDetached: posix_spawnattr_init failed: {}", IO::strerror(rc));
        closePidPipe();
        return false;
    }

    short flags = POSIX_SPAWN_SETSID
                | POSIX_SPAWN_SETSIGDEF
                | POSIX_SPAWN_SETSIGMASK;
#if defined(__APPLE__)
    flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
    rc = posix_spawnattr_setflags(&attr, flags);
    if (rc != 0) {
        spdlog::error("spawnDetached: posix_spawnattr_setflags failed: {}", IO::strerror(rc));
        posix_spawnattr_destroy(&attr);
        closePidPipe();
        return false;
    }

    sigset_t sigdef;
    sigemptyset(&sigdef);
    sigaddset(&sigdef, SIGCHLD);
    posix_spawnattr_setsigdefault(&attr, &sigdef);

    sigset_t sigmask;
    sigemptyset(&sigmask);
    posix_spawnattr_setsigmask(&attr, &sigmask);

    posix_spawn_file_actions_t fa;
    rc = posix_spawn_file_actions_init(&fa);
    if (rc != 0) {
        spdlog::error("spawnDetached: posix_spawn_file_actions_init failed: {}", IO::strerror(rc));
        posix_spawnattr_destroy(&attr);
        closePidPipe();
        return false;
    }

    posix_spawn_file_actions_addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
#if defined(__APPLE__)
    posix_spawn_file_actions_addinherit_np(&fa, STDOUT_FILENO);
    posix_spawn_file_actions_addinherit_np(&fa, STDERR_FILENO);
#endif

    // Expose the pid pipe's write end to the shell as fd 3 (see the script below). dup2 clears
    // FD_CLOEXEC on the target, so fd 3 survives exec even under POSIX_SPAWN_CLOEXEC_DEFAULT.
    if (capturePid) {
        posix_spawn_file_actions_adddup2(&fa, pidPipe[1], 3);
    }

    if (!cwd.empty()) {
        posix_spawn_file_actions_addchdir_np(&fa, cwd.c_str());
    }

    // argv = ["sh", "-c", <script>, "sh", exePath, args...]
    // The daemon is backgrounded ("$@" &) and orphaned to init/launchd. When capturing the pid, the
    // shell also prints $! (the daemon's pid) to fd 3; `3>&-` closes fd 3 in the daemon so the pipe
    // is not inherited by it and the parent's read end sees EOF once the shell exits. No caller data
    // is interpolated into the script — exePath/args arrive as positional "$@".
    static const char *kShellScript = "\"$@\" &";
    static const char *kShellScriptWithPid = "\"$@\" 3>&- & printf '%d' \"$!\" >&3";
    static const char *kShellPath = "/bin/sh";
    std::vector<const char *> shArgv;
    shArgv.reserve(args.size() + 6);
    shArgv.push_back("sh");
    shArgv.push_back("-c");
    shArgv.push_back(capturePid ? kShellScriptWithPid : kShellScript);
    shArgv.push_back("sh");
    shArgv.push_back(exePath.c_str());
    for (const auto &a : args) {
        shArgv.push_back(a.c_str());
    }
    shArgv.push_back(nullptr);

    pid_t pid = 0;
    rc = posix_spawn(&pid, kShellPath, &fa, &attr,
                     const_cast<char *const *>(shArgv.data()),
                     const_cast<char *const *>(envp.data()));

    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&attr);

    // Close our copy of the write end now that it has been handed to the child; the child's fd 3 is
    // the only writer left, so the read end will see EOF once the shell exits.
    if (pidPipe[1] >= 0) { close(pidPipe[1]); pidPipe[1] = -1; }

    if (rc != 0) {
        spdlog::error("spawnDetached: posix_spawn /bin/sh failed: {}", IO::strerror(rc));
        closePidPipe();
        return false;
    }

    // Reap the shell. It backgrounds the daemon and exits in <1ms; this waitpid is
    // bounded. The daemon itself orphans to init/launchd, which reaps it on exit.
    pid_t r;
    do {
        r = waitpid(pid, nullptr, 0);
    } while (r < 0 && errno == EINTR);
    if (r < 0) {
        spdlog::warn("spawnDetached: waitpid for shell pid={} failed: {}", (int)pid, IO::strerror(errno));
    }

    // Read the daemon pid the shell printed to fd 3. The shell writes it (a small, atomic write)
    // before exiting, so by the time waitpid() has returned the bytes are already in the pipe and a
    // single read() returns them all. A failure here is non-fatal: leave *outDaemonPid at 0 and let
    // the caller fall back to its timeout.
    if (capturePid) {
        char pidBuf[32] = {0};
        ssize_t n;
        do {
            n = read(pidPipe[0], pidBuf, sizeof(pidBuf) - 1);
        } while (n < 0 && errno == EINTR);
        if (n > 0) {
            pidBuf[n] = '\0';
            const long parsed = strtol(pidBuf, nullptr, 10);
            if (parsed > 0) {
                *outDaemonPid = static_cast<pid_t>(parsed);
            } else {
                spdlog::warn("spawnDetached: could not parse daemon pid from \"{}\"", pidBuf);
            }
        } else {
            spdlog::warn("spawnDetached: no daemon pid reported by shell");
        }
    }
    closePidPipe();
    return true;
}

void writeChildErrnoAndExit(int pipeFd, int err)
{
    (void)!write(pipeFd, &err, sizeof(err));
    _exit(127);
}

// Double-fork path for the privilege-drop case (posix_spawn has no uid/gid attribute).
//
//     helper -> fork() -> A -> setsid() -> fork() -> B -> drop privs -> execve(target)
//                              \-> _exit(0)
//
// A is the helper's direct child and exits in microseconds; helper waitpids it. B is
// reparented to init/launchd when A exits, so the daemon is never the helper's child
// and there's nothing for the helper to reap. Same orphan-to-init shape as the no-priv-
// drop /bin/sh path; differs only in needing two forks (because there's no shell to fork
// for us) and in dropping privs in B.
//
// A self-pipe with FD_CLOEXEC reports B's exec failure synchronously to the helper.
bool spawnViaForkExec(const std::string &exePath,
                      const std::vector<const char *> &argv,
                      const std::vector<const char *> &envp,
                      const std::string &cwd,
                      const std::string &runAsUser,
                      uid_t targetUid,
                      gid_t targetGid)
{
    int execErrPipe[2];
#if defined(__linux__)
    if (pipe2(execErrPipe, O_CLOEXEC) != 0) {
        spdlog::error("spawnDetached: pipe2() failed: {}", IO::strerror(errno));
        return false;
    }
#else
    if (pipe(execErrPipe) != 0) {
        spdlog::error("spawnDetached: pipe() failed: {}", IO::strerror(errno));
        return false;
    }
    fcntl(execErrPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(execErrPipe[1], F_SETFD, FD_CLOEXEC);
#endif

    pid_t pidA = fork();
    if (pidA < 0) {
        const int forkErr = errno;
        close(execErrPipe[0]);
        close(execErrPipe[1]);
        spdlog::error("spawnDetached: fork() failed: {}", IO::strerror(forkErr));
        return false;
    }

    if (pidA == 0) {
        close(execErrPipe[0]);

        if (setsid() < 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }

        pid_t pidB = fork();
        if (pidB < 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }
        if (pidB > 0) {
            _exit(0);
        }

        // B: drop privs and exec the target. Parent (A) is about to exit, so when this
        // process is alive its parent is init/launchd.
        if (!cwd.empty() && chdir(cwd.c_str()) != 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }

        if (initgroups(runAsUser.c_str(), targetGid) != 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }
        if (setgid(targetGid) != 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }
        if (setuid(targetUid) != 0) {
            writeChildErrnoAndExit(execErrPipe[1], errno);
        }

        int devnull = open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        constexpr int kMaxFdToClose = 1024;
        for (int fd = STDERR_FILENO + 1; fd < kMaxFdToClose; ++fd) {
            if (fd != execErrPipe[1]) {
                close(fd);
            }
        }

        execve(exePath.c_str(),
               const_cast<char *const *>(argv.data()),
               const_cast<char *const *>(envp.data()));
        writeChildErrnoAndExit(execErrPipe[1], errno);
    }

    close(execErrPipe[1]);

    // Reap A. It exits as soon as it's forked B (microseconds).
    pid_t r;
    do {
        r = waitpid(pidA, nullptr, 0);
    } while (r < 0 && errno == EINTR);
    if (r < 0) {
        spdlog::warn("spawnDetached: waitpid for intermediate pid={} failed: {}",
                     (int)pidA, IO::strerror(errno));
    }

    // Read B's pipe: 0 bytes = exec succeeded (FD_CLOEXEC closed the write end);
    // N bytes = either A failed before forking B, or B failed before exec.
    int childErrno = 0;
    ssize_t n;
    do {
        n = read(execErrPipe[0], &childErrno, sizeof(childErrno));
    } while (n < 0 && errno == EINTR);
    close(execErrPipe[0]);
    if (n > 0) {
        spdlog::error("spawnDetached: child failed before exec: {}", IO::strerror(childErrno));
        return false;
    }
    return true;
}

} // namespace

bool spawnDetached(const std::string &exePath,
                   const std::vector<std::string> &args,
                   const Options &opts,
                   pid_t *outDaemonPid)
{
    // The privilege-drop (double-fork) path does not report a pid; default to 0 ("unknown") here so
    // callers on that path get a consistent value.
    if (outDaemonPid) {
        *outDaemonPid = 0;
    }

    uid_t targetUid = 0;
    gid_t targetGid = 0;
    const bool dropPrivs = !opts.runAsUser.empty();
    if (dropPrivs) {
        // getpwnam returns NULL both for "user not found" (errno unchanged) and for I/O
        // errors (errno set). Clear errno first so we can tell them apart and log usefully
        // instead of printing "Success" for a missing user.
        errno = 0;
        struct passwd *pw = getpwnam(opts.runAsUser.c_str());
        if (!pw) {
            if (errno != 0) {
                spdlog::error("spawnDetached: getpwnam(\"{}\") failed: {}",
                              opts.runAsUser, IO::strerror(errno));
            } else {
                spdlog::error("spawnDetached: user \"{}\" not found", opts.runAsUser);
            }
            return false;
        }
        targetUid = pw->pw_uid;
        targetGid = pw->pw_gid;
    }

    std::vector<std::string> envStorage = buildEnvStorage(opts.extraEnv);
    std::vector<const char *> envp;
    envp.reserve(envStorage.size() + 1);
    for (const auto &e : envStorage) {
        envp.push_back(e.c_str());
    }
    envp.push_back(nullptr);

    if (!dropPrivs) {
        return spawnViaPosixSpawn(exePath, args, envp, opts.cwd, outDaemonPid);
    }

    std::vector<const char *> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(exePath.c_str());
    for (const auto &a : args) {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);
    return spawnViaForkExec(exePath, argv, envp, opts.cwd,
                            opts.runAsUser, targetUid, targetGid);
}

} // namespace Spawn
