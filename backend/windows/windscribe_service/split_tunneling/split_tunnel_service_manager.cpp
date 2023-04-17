#include "split_tunnel_service_manager.h"

#include <system_error>

#include "../../../client/common/utils/servicecontrolmanager.h"
#include "../logger.h"

static const wchar_t* kServiceName = L"WindscribeSplitTunnel";

SplitTunnelServiceManager::SplitTunnelServiceManager()
{
}

bool SplitTunnelServiceManager::start() const
{
    Logger::instance().out(L"SplitTunnelServiceManager::start()");

    bool result = true;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName, SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService();
    }
    catch (std::system_error& ex) {
        result = false;
        Logger::instance().out("SplitTunnelServiceManager::start - %s", ex.what());
    }

    return result;
}

void SplitTunnelServiceManager::stop() const
{
    Logger::instance().out(L"SplitTunnelServiceManager::stop()");

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
        scm.stopService();
    }
    catch (std::system_error& ex) {
        Logger::instance().out("SplitTunnelServiceManager::stop - %s", ex.what());
    }
}
