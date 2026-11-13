#include <cerrno>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <string>
#include <vector>

#include "../UdpSocket.h"
#include "testhelpers.h"

using namespace testhelpers;

namespace {

struct Collector
{
    std::mutex mutex;
    std::vector<UdpSocket::Datagram> datagrams;
    std::vector<UdpSocket::ResumeReading> pendingResumes;
    int batches = 0;
    bool autoResume = true;

    size_t count()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return datagrams.size();
    }

    UdpSocket::BatchHandler handler(const std::shared_ptr<Collector> &self)
    {
        return [self](std::vector<UdpSocket::Datagram> batch, UdpSocket::ResumeReading resume) {
            bool resumeNow = false;
            {
                std::lock_guard<std::mutex> lock(self->mutex);
                ++self->batches;
                for (auto &datagram : batch) {
                    self->datagrams.push_back(std::move(datagram));
                }
                resumeNow = self->autoResume;
                if (!resumeNow) {
                    self->pendingResumes.push_back(resume);
                }
            }
            if (resumeNow) {
                resume();
            }
        };
    }
};

dispatch_queue_t g_queue;
unsigned int g_loopback = 0;

std::string payloadOf(const UdpSocket::Datagram &datagram)
{
    return std::string(datagram.data.begin(), datagram.data.end());
}

bool sendFrom(UdpSocket &socket, Peer &peer, const std::string &payload)
{
    sockaddr_storage to = loopbackAddress(peer.family(), peer.port());
    return socket.sendTo(payload.data(), payload.size(), reinterpret_cast<sockaddr *>(&to), to.ss_len) == 0;
}

void testOneSocketManyPeers()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    VERIFY(relay->localPort() != 0);

    Peer peers[3];
    for (int i = 0; i < 3; ++i) {
        VERIFY(sendFrom(*relay, peers[i], "hello-" + std::to_string(i)));
    }
    for (int i = 0; i < 3; ++i) {
        std::string payload;
        uint16_t fromPort = 0;
        VERIFY(peers[i].receive(payload, fromPort));
        VERIFY(payload == "hello-" + std::to_string(i));
        VERIFY(fromPort == relay->localPort());
    }
}

void testUncontactedPeerIsDelivered()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    auto collector = std::make_shared<Collector>();
    relay->startReading(g_queue, collector->handler(collector));

    Peer stranger;
    VERIFY(stranger.sendTo(relay->localPort(), "unsolicited"));
    VERIFY(waitUntil([&] { return collector->count() == 1; }));
    std::lock_guard<std::mutex> lock(collector->mutex);
    if (collector->datagrams.size() == 1) {
        VERIFY(payloadOf(collector->datagrams[0]) == "unsolicited");
        VERIFY(portOf(collector->datagrams[0].from) == stranger.port());
    }
}

void testManyDestinationsUseNoExtraFiles()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    const int before = settledOpenFileCount(g_queue);
    int failures = 0;
    for (uint16_t port = 41000; port < 43000; ++port) {
        sockaddr_storage to = loopbackAddress(AF_INET, port);
        if (relay->sendTo("x", 1, reinterpret_cast<sockaddr *>(&to), to.ss_len) != 0) {
            ++failures;
        }
    }
    VERIFY(failures == 0);
    VERIFY_EQUAL(openFileCount(), before);

    Peer peer;
    VERIFY(sendFrom(*relay, peer, "still-works"));
    std::string payload;
    uint16_t fromPort = 0;
    VERIFY(peer.receive(payload, fromPort));
    VERIFY(payload == "still-works");
}

void testIpv6RoundTrip()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET6, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    VERIFY(relay->family() == AF_INET6);
    auto collector = std::make_shared<Collector>();
    relay->startReading(g_queue, collector->handler(collector));

    Peer peer(AF_INET6);
    VERIFY(sendFrom(*relay, peer, "v6-out"));
    std::string payload;
    uint16_t fromPort = 0;
    VERIFY(peer.receive(payload, fromPort));
    VERIFY(payload == "v6-out");
    VERIFY(fromPort == relay->localPort());

    VERIFY(peer.sendTo(relay->localPort(), "v6-in"));
    VERIFY(waitUntil([&] { return collector->count() == 1; }));
    std::lock_guard<std::mutex> lock(collector->mutex);
    if (collector->datagrams.size() == 1) {
        VERIFY(payloadOf(collector->datagrams[0]) == "v6-in");
        VERIFY(collector->datagrams[0].from.ss_family == AF_INET6);
        VERIFY(portOf(collector->datagrams[0].from) == peer.port());
    }
}

void testInterfaceBinding()
{
    int error = 0;
    auto bound = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(bound != nullptr);
    VERIFY(error == 0);
    const int before = settledOpenFileCount(g_queue);

    auto missing = UdpSocket::create(AF_INET, 999999, error);
    VERIFY(missing == nullptr);
    VERIFY(error != 0);

    missing = UdpSocket::create(AF_INET6, 999999, error);
    VERIFY(missing == nullptr);
    VERIFY(error != 0);
    VERIFY_EQUAL(openFileCount(), before);
}

void testBroadcastIsAllowed()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, 0, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    sockaddr_in to;
    std::memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_len = sizeof(to);
    to.sin_port = htons(9);
    to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    VERIFY(relay->sendTo("x", 1, reinterpret_cast<sockaddr *>(&to), sizeof(to)) != EACCES);
}

void testReadingContinuesPastFullBatch()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    Peer peer;
    for (int i = 0; i < 40; ++i) {
        VERIFY(peer.sendTo(relay->localPort(), std::to_string(i)));
    }
    auto collector = std::make_shared<Collector>();
    collector->autoResume = false;
    relay->startReading(g_queue, collector->handler(collector));
    VERIFY(waitUntil([&] { return collector->count() == 32; }));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    VERIFY_EQUAL(static_cast<long>(collector->count()), 32);

    UdpSocket::ResumeReading resume;
    {
        std::lock_guard<std::mutex> lock(collector->mutex);
        if (!collector->pendingResumes.empty()) {
            resume = collector->pendingResumes.back();
            collector->pendingResumes.clear();
        }
        collector->autoResume = true;
    }
    VERIFY(static_cast<bool>(resume));
    if (resume) {
        resume();
    }
    VERIFY(waitUntil([&] { return collector->count() == 40; }));
    std::lock_guard<std::mutex> lock(collector->mutex);
    VERIFY_EQUAL(collector->batches, 2);
    if (collector->datagrams.size() == 40) {
        for (int i = 0; i < 40; ++i) {
            VERIFY(payloadOf(collector->datagrams[i]) == std::to_string(i));
        }
    }
}

void testReadingPausesUntilResumed()
{
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    auto collector = std::make_shared<Collector>();
    collector->autoResume = false;
    relay->startReading(g_queue, collector->handler(collector));

    Peer peer;
    VERIFY(peer.sendTo(relay->localPort(), "first"));
    VERIFY(waitUntil([&] { return collector->count() == 1; }));
    VERIFY(peer.sendTo(relay->localPort(), "second"));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    VERIFY(collector->count() == 1);

    UdpSocket::ResumeReading resume;
    {
        std::lock_guard<std::mutex> lock(collector->mutex);
        VERIFY(collector->pendingResumes.size() == 1);
        if (!collector->pendingResumes.empty()) {
            resume = collector->pendingResumes.back();
            collector->pendingResumes.clear();
        }
        collector->autoResume = true;
    }
    if (resume) {
        resume();
    }
    VERIFY(waitUntil([&] { return collector->count() == 2; }));
    std::lock_guard<std::mutex> lock(collector->mutex);
    if (collector->datagrams.size() == 2) {
        VERIFY(payloadOf(collector->datagrams[1]) == "second");
    }
}

void testDestroyWhilePausedClosesAndLateResumeIsHarmless()
{
    const int before = settledOpenFileCount(g_queue);
    int error = 0;
    auto relay = UdpSocket::create(AF_INET, g_loopback, error);
    VERIFY(relay != nullptr);
    if (!relay) {
        return;
    }
    VERIFY_EQUAL(openFileCount(), before + 1);
    auto collector = std::make_shared<Collector>();
    collector->autoResume = false;
    relay->startReading(g_queue, collector->handler(collector));

    Peer peer;
    const int withPeer = openFileCount();
    VERIFY(peer.sendTo(relay->localPort(), "paused"));
    VERIFY(waitUntil([&] { return collector->count() == 1; }));

    relay.reset();
    VERIFY(waitUntil([&] { return openFileCount() == withPeer - 1; }));

    UdpSocket::ResumeReading resume;
    {
        std::lock_guard<std::mutex> lock(collector->mutex);
        if (!collector->pendingResumes.empty()) {
            resume = collector->pendingResumes.back();
        }
    }
    VERIFY(static_cast<bool>(resume));
    if (resume) {
        resume();
    }
    dispatch_sync(g_queue, ^{});
    VERIFY(collector->count() == 1);
}

}

int main()
{
    g_queue = dispatch_queue_create("udpsocket.test", DISPATCH_QUEUE_SERIAL);
    g_loopback = if_nametoindex("lo0");
    VERIFY(g_loopback != 0);

    testOneSocketManyPeers();
    testUncontactedPeerIsDelivered();
    testManyDestinationsUseNoExtraFiles();
    testIpv6RoundTrip();
    testInterfaceBinding();
    testBroadcastIsAllowed();
    testReadingContinuesPastFullBatch();
    testReadingPausesUntilResumed();
    testDestroyWhilePausedClosesAndLateResumeIsHarmless();

    if (g_failures == 0) {
        printf("PASS\n");
    }
    return g_failures;
}
