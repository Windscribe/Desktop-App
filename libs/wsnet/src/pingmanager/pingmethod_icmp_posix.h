#pragma once

#include "ipingmethod.h"
#include "processmanager.h"

namespace wsnet {

class PingMethodIcmp_posix : public IPingMethod
{
public:
    PingMethodIcmp_posix(std::uint64_t id, const std::string &ip, const std::string &hostname, bool isParallelPing,
                    PingFinishedCallback callback, PingMethodFinishedCallback pingMethodFinishedCallback, ProcessManager *processManager);

    virtual ~PingMethodIcmp_posix();
    void ping(bool isFromDisconnectedVpnState) override;

private:
    ProcessManager *processManager_;

    void onProcessFinished(int exitCode, const std::string &output);
    int extractTimeMs(const std::string &str);
};

} // namespace wsnet
