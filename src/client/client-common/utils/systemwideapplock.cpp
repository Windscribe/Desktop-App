#include "systemwideapplock.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#endif

namespace {

#if defined(_WIN32)

std::wstring toWide(const std::string &name)
{
    return std::wstring(name.begin(), name.end());
}

#else

bool processOwner(pid_t pid, uid_t &uid)
{
#if defined(__APPLE__)
    struct kinfo_proc kp;
    size_t len = sizeof(kp);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    if (sysctl(mib, 4, &kp, &len, nullptr, 0) != 0 || len == 0) {
        return false;
    }
    uid = kp.kp_eproc.e_ucred.cr_uid;
    return true;
#else
    // Not the owner of /proc/<pid>: the app is setgid, which makes the kernel hand that to root.
    std::ifstream status("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("Uid:", 0) == 0) {
            char *end = nullptr;
            const unsigned long value = strtoul(line.c_str() + 4, &end, 10);
            if (end == line.c_str() + 4) {
                return false;
            }
            uid = static_cast<uid_t>(value);
            return true;
        }
    }
    return false;
#endif
}

SystemWideAppLock::Holder lockHolder(int fd)
{
    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    if (fcntl(fd, F_GETLK, &fl) != 0) {
        return SystemWideAppLock::Holder::kUnknown;
    }
    if (fl.l_type == F_UNLCK) {
        return SystemWideAppLock::Holder::kNone;
    }
    // A holder that cannot be identified still holds the lock, so it is never reported as this user.
    uid_t uid;
    if (processOwner(fl.l_pid, uid) && uid == getuid()) {
        return SystemWideAppLock::Holder::kThisUser;
    }
    return SystemWideAppLock::Holder::kOtherUser;
}

#endif

} // namespace

std::string SystemWideAppLock::defaultName()
{
#if defined(_WIN32)
    return "Global\\" WS_APP_IDENTIFIER "AppRunning";
#elif defined(__APPLE__)
    return WS_MAC_APP_LOCK_FILE;
#else
    return WS_LINUX_APP_LOCK_FILE;
#endif
}

SystemWideAppLock::SystemWideAppLock(const std::string &name) : name_(name)
{
}

SystemWideAppLock::~SystemWideAppLock()
{
#if defined(_WIN32)
    if (event_) {
        CloseHandle(static_cast<HANDLE>(event_));
    }
#else
    if (fd_ >= 0) {
        close(fd_);
    }
#endif
}

#if defined(_WIN32)

SystemWideAppLock::Holder SystemWideAppLock::acquire()
{
    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, toWide(name_).c_str());
    const DWORD err = GetLastError();
    if (event && err == ERROR_ALREADY_EXISTS) {
        CloseHandle(event);
        return Holder::kThisUser;
    }
    if (!event) {
        // The event's default security admits only the user who created it.
        return err == ERROR_ACCESS_DENIED ? Holder::kOtherUser : Holder::kUnknown;
    }
    event_ = event;
    return Holder::kNone;
}

SystemWideAppLock::Holder SystemWideAppLock::holder(const std::string &name)
{
    HANDLE event = OpenEventW(SYNCHRONIZE, FALSE, toWide(name).c_str());
    if (event) {
        CloseHandle(event);
        return Holder::kThisUser;
    }
    const DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND) {
        return Holder::kNone;
    }
    return err == ERROR_ACCESS_DENIED ? Holder::kOtherUser : Holder::kUnknown;
}

#else

SystemWideAppLock::Holder SystemWideAppLock::acquire()
{
    const int fd = open(name_.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return Holder::kUnknown;
    }

    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    // Twice, in case the holder lets go between the failed take and the query.
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (fcntl(fd, F_SETLK, &fl) == 0) {
            fd_ = fd;
            return Holder::kNone;
        }
        if (errno != EAGAIN && errno != EACCES) {
            break;
        }
        const Holder result = lockHolder(fd);
        if (result != Holder::kNone) {
            close(fd);
            return result;
        }
    }
    close(fd);
    return Holder::kUnknown;
}

SystemWideAppLock::Holder SystemWideAppLock::holder(const std::string &name)
{
    const int fd = open(name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return Holder::kUnknown;
    }
    const Holder result = lockHolder(fd);
    close(fd);
    return result;
}

#endif
