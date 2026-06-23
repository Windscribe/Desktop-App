#include "helperconnector_linux.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "utils/log/categories.h"

namespace {

constexpr int kMaxWaitHelperMs = 5000;

// Handoff state for the fd produced by the detached connect thread. Intentionally heap-allocated and
// never freed (see state()): the connect thread is detached and can still touch this after main()
// returns, so it must outlive static destruction.
struct State
{
    std::mutex mutex;
    bool started = false;     // startConnect() spawned the connect thread
    bool ready = false;       // the connect thread has finished
    bool abandoned = false;   // the consumer gave up; close the fd instead of holding it
    int fd = -1;
};

State &state()
{
    static State *s = new State();
    return *s;
}

bool setBlocking(int fd)
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == 0;
}

// Connects to the helper socket, retrying for up to kMaxWaitHelperMs if it is not yet accepting.
// Returns a connected, blocking fd, or -1 on failure. Connect is non-blocking + poll so a wedged
// listener can't hang this thread past the deadline.
int connectWithRetry()
{
    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (std::strlen(WS_LINUX_HELPER_SOCKET) >= sizeof(addr.sun_path)) {
        qCCritical(LOG_BASIC) << "Helper socket path too long";
        return -1;
    }
    std::strncpy(addr.sun_path, WS_LINUX_HELPER_SOCKET, sizeof(addr.sun_path) - 1);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kMaxWaitHelperMs);
    for (;;) {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) {
            qCCritical(LOG_BASIC) << "Could not create helper socket:" << errno;
            return -1;
        }

        bool connected = false;
        if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
            connected = true;
        } else if (errno == EINPROGRESS) {
            const auto now = std::chrono::steady_clock::now();
            const int timeoutMs = now < deadline
                ? static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count())
                : 0;
            pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            if (::poll(&pfd, 1, timeoutMs) > 0) {
                int err = 0;
                socklen_t len = sizeof(err);
                if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                    connected = true;
                }
            }
        }

        if (connected) {
            if (setBlocking(fd)) {
                return fd;
            }
            ::close(fd);
            return -1;
        }

        ::close(fd);
        if (std::chrono::steady_clock::now() >= deadline) {
            qCCritical(LOG_BASIC) << "Timed out connecting to helper socket";
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Stores the connect result. If the consumer already abandoned the connect, close the fd here so it
// is not leaked.
void onConnectFinished(int fd)
{
    State &s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.ready = true;
    if (s.abandoned) {
        if (fd >= 0) {
            ::close(fd);
        }
        return;
    }
    s.fd = fd;
}

}  // namespace

void HelperConnector::startConnect()
{
    State &s = state();
    {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (s.started) {
            return;
        }
        s.started = true;
    }
    std::thread([]() {
        onConnectFinished(connectWithRetry());
    }).detach();
}

bool HelperConnector::tryGetConnectedFd(int &outFd)
{
    State &s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (!s.started) {
        // startConnect() was never called; nothing to wait for.
        outFd = -1;
        return true;
    }
    if (!s.ready) {
        return false;
    }
    // Transfer ownership: a second caller gets -1, never the same fd twice.
    outFd = s.fd;
    s.fd = -1;
    return true;
}

void HelperConnector::abandon()
{
    State &s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.abandoned = true;
    if (s.ready && s.fd >= 0) {
        ::close(s.fd);
        s.fd = -1;
    }
}
