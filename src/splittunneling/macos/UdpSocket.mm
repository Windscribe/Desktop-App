#include "UdpSocket.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

namespace {

constexpr size_t kMaxBatch = 32;
constexpr size_t kMaxDatagram = 65536;

}

struct UdpSocket::State
{
    int fd = -1;
    dispatch_queue_t queue = nullptr;
    dispatch_source_t source = nullptr;
    bool suspended = false;
    bool cancelled = false;
    BatchHandler handler;
};

namespace {

using StatePtr = std::shared_ptr<UdpSocket::State>;

void deleteContext(void *context)
{
    delete static_cast<StatePtr *>(context);
}

void onCancel(void *context)
{
    StatePtr state = *static_cast<StatePtr *>(context);
    close(state->fd);
    state->fd = -1;
}

void resumeOnQueue(void *context)
{
    std::unique_ptr<StatePtr> holder(static_cast<StatePtr *>(context));
    StatePtr state = *holder;
    if (!state->cancelled && state->suspended) {
        state->suspended = false;
        dispatch_resume(state->source);
    }
}

void cancelOnQueue(void *context)
{
    std::unique_ptr<StatePtr> holder(static_cast<StatePtr *>(context));
    StatePtr state = *holder;
    state->cancelled = true;
    state->handler = nullptr;
    if (state->suspended) {
        state->suspended = false;
        dispatch_resume(state->source);
    }
    dispatch_source_cancel(state->source);
    state->source = nullptr;
}

void onReadable(void *context)
{
    StatePtr state = *static_cast<StatePtr *>(context);
    if (state->cancelled || state->suspended || !state->handler) {
        return;
    }

    static thread_local std::vector<uint8_t> buffer(kMaxDatagram);
    std::vector<UdpSocket::Datagram> batch;
    while (batch.size() < kMaxBatch) {
        UdpSocket::Datagram datagram;
        std::memset(&datagram.from, 0, sizeof(datagram.from));
        socklen_t fromLength = sizeof(datagram.from);
        const ssize_t received = recvfrom(state->fd, buffer.data(), buffer.size(), 0,
                                          reinterpret_cast<sockaddr *>(&datagram.from), &fromLength);
        if (received < 0) {
            break;
        }
        datagram.data.assign(buffer.begin(), buffer.begin() + received);
        batch.push_back(std::move(datagram));
    }
    if (batch.empty()) {
        return;
    }

    state->suspended = true;
    dispatch_suspend(state->source);
    UdpSocket::ResumeReading resume = [state]() {
        dispatch_async_f(state->queue, new StatePtr(state), resumeOnQueue);
    };
    state->handler(std::move(batch), std::move(resume));
}

}

std::unique_ptr<UdpSocket> UdpSocket::create(int family, unsigned int interfaceIndex, int &error)
{
    error = 0;
    const int fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        error = errno;
        return nullptr;
    }

    auto fail = [fd, &error]() {
        error = errno;
        close(fd);
        return nullptr;
    };

    const int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 || fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
        return fail();
    }

    sockaddr_storage address;
    std::memset(&address, 0, sizeof(address));
    socklen_t addressLength = 0;
    if (family == AF_INET6) {
        const int on = 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
            return fail();
        }
        if (interfaceIndex != 0 && setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &interfaceIndex, sizeof(interfaceIndex)) < 0) {
            return fail();
        }
        auto *v6 = reinterpret_cast<sockaddr_in6 *>(&address);
        v6->sin6_family = AF_INET6;
        v6->sin6_len = sizeof(sockaddr_in6);
        v6->sin6_addr = in6addr_any;
        addressLength = sizeof(sockaddr_in6);
    } else {
        const int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
            return fail();
        }
        if (interfaceIndex != 0 && setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &interfaceIndex, sizeof(interfaceIndex)) < 0) {
            return fail();
        }
        auto *v4 = reinterpret_cast<sockaddr_in *>(&address);
        v4->sin_family = AF_INET;
        v4->sin_len = sizeof(sockaddr_in);
        v4->sin_addr.s_addr = htonl(INADDR_ANY);
        addressLength = sizeof(sockaddr_in);
    }

    if (bind(fd, reinterpret_cast<sockaddr *>(&address), addressLength) < 0) {
        return fail();
    }

    return std::unique_ptr<UdpSocket>(new UdpSocket(fd, family));
}

UdpSocket::UdpSocket(int fd, int family) : fd_(fd), family_(family)
{
}

UdpSocket::~UdpSocket()
{
    if (state_) {
        dispatch_async_f(state_->queue, new StatePtr(state_), cancelOnQueue);
    } else {
        close(fd_);
    }
}

int UdpSocket::family() const
{
    return family_;
}

uint16_t UdpSocket::localPort() const
{
    sockaddr_storage address;
    socklen_t length = sizeof(address);
    if (getsockname(fd_, reinterpret_cast<sockaddr *>(&address), &length) < 0) {
        return 0;
    }
    if (address.ss_family == AF_INET6) {
        return ntohs(reinterpret_cast<sockaddr_in6 *>(&address)->sin6_port);
    }
    return ntohs(reinterpret_cast<sockaddr_in *>(&address)->sin_port);
}

int UdpSocket::sendTo(const void *data, size_t length, const sockaddr *to, socklen_t toLength)
{
    if (sendto(fd_, data, length, 0, to, toLength) < 0) {
        return errno;
    }
    return 0;
}

void UdpSocket::startReading(dispatch_queue_t queue, BatchHandler handler)
{
    if (state_) {
        return;
    }
    state_ = std::make_shared<State>();
    state_->fd = fd_;
    state_->queue = queue;
    state_->handler = std::move(handler);
    state_->source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, static_cast<uintptr_t>(fd_), 0, queue);
    dispatch_set_context(state_->source, new StatePtr(state_));
    dispatch_set_finalizer_f(state_->source, deleteContext);
    dispatch_source_set_event_handler_f(state_->source, onReadable);
    dispatch_source_set_cancel_handler_f(state_->source, onCancel);
    dispatch_activate(state_->source);
}
