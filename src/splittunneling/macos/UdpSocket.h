#pragma once

#include <cstdint>
#include <dispatch/dispatch.h>
#include <functional>
#include <memory>
#include <sys/socket.h>
#include <vector>

class UdpSocket
{
public:
    struct Datagram
    {
        std::vector<uint8_t> data;
        sockaddr_storage from;
    };
    using ResumeReading = std::function<void()>;
    using BatchHandler = std::function<void(std::vector<Datagram> batch, ResumeReading resume)>;

    static std::unique_ptr<UdpSocket> create(int family, unsigned int interfaceIndex, int &error);
    ~UdpSocket();
    UdpSocket(const UdpSocket &) = delete;
    UdpSocket &operator=(const UdpSocket &) = delete;

    int family() const;
    uint16_t localPort() const;
    int sendTo(const void *data, size_t length, const sockaddr *to, socklen_t toLength);
    void startReading(dispatch_queue_t queue, BatchHandler handler);

    struct State;

private:
    UdpSocket(int fd, int family);

    int fd_;
    int family_;
    std::shared_ptr<State> state_;
};
