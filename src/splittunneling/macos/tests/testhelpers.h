#pragma once

#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <dispatch/dispatch.h>
#include <functional>
#include <libproc.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace testhelpers {

inline int g_failures = 0;

inline bool check(bool ok, const char *expr, const char *file, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (%s:%d): %s\n", file, line, expr);
    }
    return ok;
}

#define VERIFY(expr) testhelpers::check(!!(expr), #expr, __FILE__, __LINE__)

inline bool checkEqual(long actual, long expected, const char *expr, const char *file, int line)
{
    if (actual != expected) {
        ++g_failures;
        printf("FAIL (%s:%d): %s (actual %ld, expected %ld)\n", file, line, expr, actual, expected);
        return false;
    }
    return true;
}

#define VERIFY_EQUAL(actual, expected) testhelpers::checkEqual((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)

inline int openFileCount()
{
    const int estimate = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, nullptr, 0);
    if (estimate <= 0) {
        return -1;
    }
    std::vector<proc_fdinfo> fds(static_cast<size_t>(estimate) / PROC_PIDLISTFD_SIZE + 64);
    const int size = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, fds.data(), static_cast<int>(fds.size() * PROC_PIDLISTFD_SIZE));
    return size < 0 ? -1 : size / static_cast<int>(PROC_PIDLISTFD_SIZE);
}

inline int settledOpenFileCount(dispatch_queue_t queue)
{
    int previous = -1;
    for (int attempt = 0; attempt < 40; ++attempt) {
        dispatch_sync(queue, ^{});
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const int current = openFileCount();
        if (current == previous) {
            return current;
        }
        previous = current;
    }
    return previous;
}

inline bool waitUntil(const std::function<bool()> &condition, int timeoutMs = 2000)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return condition();
}

inline sockaddr_storage loopbackAddress(int family, uint16_t port)
{
    sockaddr_storage address;
    std::memset(&address, 0, sizeof(address));
    if (family == AF_INET6) {
        auto *v6 = reinterpret_cast<sockaddr_in6 *>(&address);
        v6->sin6_family = AF_INET6;
        v6->sin6_len = sizeof(sockaddr_in6);
        v6->sin6_addr = in6addr_loopback;
        v6->sin6_port = htons(port);
    } else {
        auto *v4 = reinterpret_cast<sockaddr_in *>(&address);
        v4->sin_family = AF_INET;
        v4->sin_len = sizeof(sockaddr_in);
        v4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        v4->sin_port = htons(port);
    }
    return address;
}

inline uint16_t portOf(const sockaddr_storage &address)
{
    if (address.ss_family == AF_INET6) {
        return ntohs(reinterpret_cast<const sockaddr_in6 *>(&address)->sin6_port);
    }
    return ntohs(reinterpret_cast<const sockaddr_in *>(&address)->sin_port);
}

class Peer
{
public:
    explicit Peer(int family = AF_INET) : family_(family)
    {
        fd_ = socket(family, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_storage address = loopbackAddress(family, 0);
        bind(fd_, reinterpret_cast<sockaddr *>(&address), address.ss_len);
        socklen_t length = sizeof(address);
        getsockname(fd_, reinterpret_cast<sockaddr *>(&address), &length);
        port_ = portOf(address);
    }
    ~Peer() { close(fd_); }
    Peer(const Peer &) = delete;
    Peer &operator=(const Peer &) = delete;

    uint16_t port() const { return port_; }
    int family() const { return family_; }
    std::string host() const { return family_ == AF_INET6 ? "::1" : "127.0.0.1"; }

    bool sendTo(uint16_t port, const std::string &payload)
    {
        sockaddr_storage to = loopbackAddress(family_, port);
        return sendto(fd_, payload.data(), payload.size(), 0, reinterpret_cast<sockaddr *>(&to), to.ss_len) ==
               static_cast<ssize_t>(payload.size());
    }

    bool receive(std::string &payload, uint16_t &fromPort, int timeoutMs = 2000)
    {
        pollfd pfd = {fd_, POLLIN, 0};
        if (poll(&pfd, 1, timeoutMs) <= 0) {
            return false;
        }
        char buffer[2048];
        sockaddr_storage from;
        socklen_t length = sizeof(from);
        const ssize_t received = recvfrom(fd_, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&from), &length);
        if (received < 0) {
            return false;
        }
        payload.assign(buffer, static_cast<size_t>(received));
        fromPort = portOf(from);
        return true;
    }

private:
    int family_;
    int fd_;
    uint16_t port_ = 0;
};

}
