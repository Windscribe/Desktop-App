#include "pingmethod_icmp_win.h"

#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winternl.h>
#include <icmpapi.h>

#include "utils/wsnet_logger.h"

namespace wsnet {

PingMethodIcmp_win::PingMethodIcmp_win(EventCallbackManager_win &eventCallbackManager, std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
        PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback) :
    IPingMethod(id, ip, hostname, isParallelPing, callback, pingMethodFinishedCallback),
    eventCallbackManager_(eventCallbackManager)
{
    hEvent_ = CreateEvent(NULL, true, false, NULL);
}

PingMethodIcmp_win::~PingMethodIcmp_win()
{
    eventCallbackManager_.remove(hEvent_);
    CloseHandle(hEvent_);
    if (icmpFile_ != INVALID_HANDLE_VALUE)
        IcmpCloseHandle(icmpFile_);
}

void PingMethodIcmp_win::ping(bool isFromDisconnectedVpnState)
{
    isFromDisconnectedVpnState_ = isFromDisconnectedVpnState;

    icmpFile_ = ::IcmpCreateFile();
    if (icmpFile_ == INVALID_HANDLE_VALUE) {
        g_logger->error("PingHost_ICMP_win IcmpCreateFile failed, error: {}", ::GetLastError());
        assert(false);
        callFinished();
        return;
    }

    const char dataForSend[] = "HelloBufferBuffer";
    replySize_ = sizeof(ICMP_ECHO_REPLY) + sizeof(dataForSend) + 8;
    replyBuffer_.reset(new unsigned char[replySize_]);

    IN_ADDR ipAddr;
    if (inet_pton(AF_INET, ip_.c_str(), &ipAddr) != 1) {
        g_logger->error("PingHost_ICMP_win inet_pton failed, error: {}", ::WSAGetLastError());
        callFinished();
        return;
    }

    startTime_ = std::chrono::steady_clock::now();

    // Attach an event to a callback function
    eventCallbackManager_.add(hEvent_, std::bind(&PingMethodIcmp_win::onCallback, this));

    DWORD result = ::IcmpSendEcho2(icmpFile_, hEvent_, NULL, NULL, ipAddr.S_un.S_addr,
                                   (LPVOID)dataForSend, sizeof(dataForSend), NULL,
                                   replyBuffer_.get(), replySize_, 2000);
    if (result != 0) {
        g_logger->error("PingHost_ICMP_win IcmpSendEcho2 returned unexpected result, error: {}", ::GetLastError());
        assert(false);
        callFinished();
        return;
    }
    else if (::GetLastError() != ERROR_IO_PENDING) {
        // This is normal behaviour, can return the result immediately without a callback
        callFinished();
        return;
    }
}

void PingMethodIcmp_win::onCallback()
{
    if (::IcmpParseReplies(replyBuffer_.get(), replySize_) > 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer_.get();
        if (pEchoReply->Status == IP_SUCCESS) {
            isSuccess_ = true;
            timeMs_ = pEchoReply->RoundTripTime;
        }
    }
    callFinished();
}

} // namespace wsnet
