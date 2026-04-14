#include "bfe_service_win.h"

#include "utils/log/categories.h"
#include "utils/servicecontrolmanager.h"

namespace BFEService
{

bool isRunning(Helper *helper)
{
    DWORD status = 0; // Status undetermined
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(L"BFE", SERVICE_QUERY_STATUS);
        status = scm.queryServiceStatus();
    }
    catch (std::system_error& ex) {
        qCWarning(LOG_BASIC) << "BFEService::isRunning -" << ex.what();
    }

    // We were unable to query the status of the service, likely due to third-party security
    // software blocking/restricting our access to the SCM.  See if the elevated helper process
    // can do it.
    if (status == 0) {
        status = helper->queryBFEStatus();
    }

    if (status != SERVICE_RUNNING) {
        qCWarning(LOG_BASIC) << "BFEService::isRunning - BFE service is" << wsl::ServiceControlManager::serviceStatusToString(status);
        return false;
    }

    return true;
}

bool start(Helper *helper)
{
    return helper->enableBFE();
}

}
