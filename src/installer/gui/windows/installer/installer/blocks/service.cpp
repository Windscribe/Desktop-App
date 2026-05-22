#include "service.h"

#include <Windows.h>
#include <fwpmu.h>

#include <sstream>
#include <spdlog/spdlog.h>

#include "installerenums.h"
#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "servicecontrolmanager.h"

using namespace std;

Service::Service(double weight) : IInstallBlock(weight, L"Service")
{
}

int Service::executeStep()
{
    int result = verifyBFEStatus();
    if (result != 0) {
        return result;
    }

    result = installWindscribeService();
    if (result != 0) {
        return result;
    }

    return 100;
}

int Service::installWindscribeService()
{
    int result = -wsl::ERROR_HELPER_INSTALL;
    try {
        std::wstring serviceName = ApplicationInfo::serviceName();
        std::wstring serviceCmdLine;
        {
            std::wostringstream stream;
            stream << L"\"" << Settings::instance().getPath() << L"\\WindscribeService.exe\"";
            serviceCmdLine = stream.str();
        }

        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_ALL_ACCESS);

        if (scm.isServiceInstalled(serviceName.c_str())) {
            spdlog::warn("The Windscribe service is already installed, attempting to delete it.");
            std::error_code ec;
            if (!scm.deleteService(serviceName.c_str(), ec)) {
                spdlog::error("Failed to remove existing Windscribe service - {}", ec.message());
            }
        }

        scm.installService(serviceName.c_str(), serviceCmdLine.c_str(),
                           L"Windscribe Service", L"Manages the firewall and controls the VPN tunnel",
                           SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, L"BFE\0Nsi\0TcpIp\0", true);

        scm.setServiceSIDType(SERVICE_SID_TYPE_UNRESTRICTED);

        result = -wsl::ERROR_HELPER_START;
        scm.openService(serviceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService();

        result = 0;
    }
    catch (const std::system_error& ex) {
        spdlog::error("installWindscribeService - {}", ex.what());
    }

    return result;
}

int Service::verifyBFEStatus()
{
    // Verify the BFE service is running and we can connect to it.  There is no use proceeding with the install
    // if we cannot access this core firewall service, since the helper will fail to start if it cannot connect
    // to it.

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(L"BFE", SERVICE_QUERY_STATUS | SERVICE_START);
        const auto status = scm.queryServiceStatus();
        if (status != SERVICE_RUNNING) {
            spdlog::warn("verifyBFEStatus - BFE is not running, attempting to start it.");
            scm.startService();
        }

        HANDLE engineHandle = NULL;
        const auto result = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engineHandle);
        if (result != ERROR_SUCCESS) {
            spdlog::error("verifyBFEStatus - FwpmEngineOpen0 failed {}", result);
            return -wsl::ERROR_BFE_SERVICE_CONNECTION;
        }

        FwpmEngineClose0(engineHandle);
    }
    catch (std::system_error& ex) {
        spdlog::error("verifyBFEStatus - {}", ex.what());
        return -wsl::ERROR_BFE_SERVICE_NOT_RUNNING;
    }

    return 0;
}
