#include "ws_branding.h"
#include "split_tunnel_service_manager.h"

#include <system_error>
#include <spdlog/spdlog.h>

#include "../../client/client-common/utils/servicecontrolmanager.h"


static const wchar_t* kServiceName = WS_APP_IDENTIFIER_W L"SplitTunnel";

SplitTunnelServiceManager::SplitTunnelServiceManager()
{
}

bool SplitTunnelServiceManager::start()
{
    spdlog::info("SplitTunnelServiceManager::start()");

    // Cleared even if the start fails: the driver may have started anyway.
    isKnownStopped_ = false;

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

void SplitTunnelServiceManager::stop()
{
    // Every connect status update with split tunneling off lands here. Asking the SCM to stop a
    // driver that is already stopped costs ~20 ms each time, so skip it once a stop has succeeded.
    if (isKnownStopped_) {
        return;
    }

    spdlog::info("SplitTunnelServiceManager::stop()");

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
        scm.stopService();
        isKnownStopped_ = true;
    }
    catch (std::system_error& ex) {
        spdlog::error("SplitTunnelServiceManager::stop - {}", ex.what());
    }
}
