#pragma once

#include <string>

// A lock shared by every user on the machine, held for as long as the app runs. Only one user's app
// may drive the helper at a time: a second user's app would tear down the first user's VPN.
// The OS releases the lock when the holding process exits or crashes.
class SystemWideAppLock
{
public:
    enum class Holder { kNone, kThisUser, kOtherUser, kUnknown };

    // The lock file on macOS and Linux, the event name on Windows. Tests pass their own.
    static std::string defaultName();

    explicit SystemWideAppLock(const std::string &name = defaultName());
    ~SystemWideAppLock();

    SystemWideAppLock(const SystemWideAppLock &) = delete;
    SystemWideAppLock &operator=(const SystemWideAppLock &) = delete;

    // Takes the lock for the life of this object. kNone means it is now ours; kUnknown means the lock
    // could not be used at all, in which case the app runs unprotected rather than not at all.
    Holder acquire();

    // Who holds the lock, without taking it. Never call this in a process that holds the lock: on
    // macOS and Linux, closing the file it opens drops that process's lock.
    static Holder holder(const std::string &name = defaultName());

private:
    std::string name_;
#if defined(_WIN32)
    void *event_ = nullptr;
#else
    int fd_ = -1;
#endif
};
