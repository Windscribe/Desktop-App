#include "pingmethod_icmp_posix.h"
#include "utils/wsnet_logger.h"
#include "utils/utils.h"

namespace wsnet {

PingMethodIcmp_posix::PingMethodIcmp_posix(std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
        PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback, ProcessManager *processManager) :
    IPingMethod(id, ip, hostname, isParallelPing, callback, pingMethodFinishedCallback),
    processManager_(processManager)
{
}

PingMethodIcmp_posix::~PingMethodIcmp_posix()
{
}

void PingMethodIcmp_posix::ping(bool isFromDisconnectedVpnState)
{
    if (!utils::isIpAddress(ip_)) {
        g_logger->error("PingMethodIcmp_posix::ping incorrect IP-address: {}", ip_);
        callFinished();
        return;
    }

    using namespace std::placeholders;
    isFromDisconnectedVpnState_ = isFromDisconnectedVpnState;

    if (!processManager_->execute("ping", {"-c", "1", "-W", "2000", ip_}, std::bind(&PingMethodIcmp_posix::onProcessFinished, this, _1, _2))) {
        g_logger->error("PingMethodIcmp_posix::ping cannot execute ping command");
        callFinished();
        return;
    }
}

void PingMethodIcmp_posix::onProcessFinished(int exitCode, const std::string &output)
{
    if (exitCode == 0) {
        auto lines = utils::split(output, "\n");
        for (const auto &line : lines) {
            if (line.find("icmp_seq=") != std::string::npos) {
                auto ind = line.find("time=");
                if (ind != std::string::npos) {
                    int t = extractTimeMs(line.substr(ind, line.length() - ind));
                    if (t != -1) {
                        timeMs_ = t;
                        isSuccess_ = true;
                    }
                } else {
                    g_logger->error("Something incorrect in ping utility output: {}", output);
                    assert(false);
                }
                break;
            }
        }
    } else if (exitCode != 2) {
        g_logger->error("ping utility return not 0 or 2 exitCode, exitCode: {}", exitCode);
    }
    callFinished();
}

int PingMethodIcmp_posix::extractTimeMs(const std::string &str)
{
    auto ind = str.find('=');
    if (ind != std::string::npos) {
        std::string val;
        ind++;
        while (ind < str.length() && !std::isspace(str[ind])) {
            val += str[ind];
            ind++;
        }
        return (int)std::stof(val);
    }

    return -1;
}


} // namespace wsnet
