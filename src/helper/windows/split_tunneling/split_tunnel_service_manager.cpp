#include "split_tunnel_service_manager.h"

#include <system_error>
#include <spdlog/spdlog.h>

#include "../../../client/common/utils/servicecontrolmanager.h"


static const wchar_t* kServiceName = L"WindscribeSplitTunnel";

SplitTunnelServiceManager::SplitTunnelServiceManager()
{
}

bool SplitTunnelServiceManager::start() const
{
    spdlog::info("SplitTunnelServiceManager::start()");

    bool result = true;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName, SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService();
    }
    catch (std::system_error& ex) {
        result = false;
        spdlog::error("SplitTunnelServiceManager::start - {}", ex.what());
    }

    return result;
}

void SplitTunnelServiceManager::stop() const
{
    spdlog::info("SplitTunnelServiceManager::stop()");

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
        scm.stopService();
    }
    catch (std::system_error& ex) {
        spdlog::error("SplitTunnelServiceManager::stop - {}", ex.what());
    }
}
