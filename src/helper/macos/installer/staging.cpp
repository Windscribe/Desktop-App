#include "staging.h"

#include <cerrno>
#include <copyfile.h>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

extern char **environ;

namespace Staging
{

bool freezeRegularFile(const std::string &srcPath, const std::string &dstPath)
{
    // O_NOFOLLOW rejects a symlink swapped in at the source path; O_NONBLOCK keeps a planted FIFO or
    // device from blocking the open (both are rejected by the S_ISREG check). A hard link to a
    // root-only file is read here, but the caller's signature check on the copy rejects it.
    int srcFd = open(srcPath.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    if (srcFd < 0) {
        spdlog::error("freezeRegularFile: open \"{}\" failed: {}", srcPath, strerror(errno));
        return false;
    }
    struct stat srcSt;
    if (fstat(srcFd, &srcSt) != 0 || !S_ISREG(srcSt.st_mode)) {
        spdlog::error("freezeRegularFile: \"{}\" is not a regular file", srcPath);
        close(srcFd);
        return false;
    }
    // O_EXCL | O_NOFOLLOW: dst must not pre-exist, so root never writes through an attacker-planted
    // file or symlink at the destination.
    int dstFd = open(dstPath.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (dstFd < 0) {
        spdlog::error("freezeRegularFile: create \"{}\" failed: {}", dstPath, strerror(errno));
        close(srcFd);
        return false;
    }
    // Force 0600 regardless of the process umask, which would otherwise mask the open mode.
    fchmod(dstFd, S_IRUSR | S_IWUSR);
    int rc = fcopyfile(srcFd, dstFd, NULL, COPYFILE_DATA);
    int copyErr = errno;
    int syncRc = fsync(dstFd);
    int syncErr = errno;
    close(dstFd);
    close(srcFd);
    if (rc != 0) {
        spdlog::error("freezeRegularFile: copy \"{}\" -> \"{}\" failed: {}", srcPath, dstPath, strerror(copyErr));
        return false;
    }
    // A failed fsync can leave the on-disk copy short; the caller wipes the stage on false.
    if (syncRc != 0) {
        spdlog::error("freezeRegularFile: fsync \"{}\" failed: {}", dstPath, strerror(syncErr));
        return false;
    }
    return true;
}

int runWithTimeout(const std::string &exePath, const std::vector<std::string> &args, int timeoutSecs)
{
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(exePath.c_str()));
    for (const auto &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // CLOEXEC_DEFAULT closes every inherited fd in the child; redirect std streams to /dev/null so the
    // child never runs with 0/1/2 closed (a classic fd-reuse footgun) and we don't depend on the
    // daemon's own 1/2 being open. hdiutil output is not consumed; the caller logs the exit code.
    posix_spawnattr_t attr;
    if (posix_spawnattr_init(&attr) != 0) {
        return -1;
    }
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        posix_spawnattr_destroy(&attr);
        return -1;
    }
    int setupRc = posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT);
    setupRc |= posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    setupRc |= posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    setupRc |= posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid = 0;
    int spawnRc = setupRc != 0 ? setupRc
                               : posix_spawn(&pid, exePath.c_str(), &actions, &attr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);
    if (spawnRc != 0) {
        return -1;
    }

    // Track elapsed against a monotonic clock rather than accumulated poll ticks, which drift under EINTR.
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    auto elapsedMs = [&start]() -> long {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
    };

    constexpr int kPollMs = 50;
    const struct timespec pollTs { 0, kPollMs * 1000000L };
    for (;;) {
        int status = 0;
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (elapsedMs() >= static_cast<long>(timeoutSecs) * 1000) {
            kill(pid, SIGKILL);
            // Bounded reap so a child wedged in uninterruptible I/O can't re-hang us under the caller's lock.
            for (int reapMs = 0; reapMs < 2000; reapMs += kPollMs) {
                if (waitpid(pid, nullptr, WNOHANG) != 0) {
                    break;
                }
                nanosleep(&pollTs, nullptr);
            }
            return -2;
        }
        nanosleep(&pollTs, nullptr);
    }
}

}
