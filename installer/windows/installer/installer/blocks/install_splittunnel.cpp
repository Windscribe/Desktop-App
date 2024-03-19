#include "install_splittunnel.h"

#include <filesystem>

#include "../settings.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"

using namespace std;

InstallSplitTunnel::InstallSplitTunnel(double weight) : IInstallBlock(weight, L"splittunnel", false)
{
}

int InstallSplitTunnel::executeStep()
{
    int result = 100;

    SC_HANDLE hSCM = NULL;
    SC_HANDLE hService = NULL;

    try {
        error_code ec;
        wstring sourceFile = Path::append(Settings::instance().getPath(), L"splittunnel\\windscribesplittunnel.sys");
        wstring targetFile = Path::append(Utils::GetSystemDir(), L"drivers\\windscribesplittunnel.sys");
        filesystem::copy(sourceFile, targetFile, filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            throw system_error(ec, "InstallSplitTunnel failed to copy driver to system drivers folder");
        }

        hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

        if (hSCM == NULL) {
            throw system_error(::GetLastError(), system_category(), "InstallSplitTunnel OpenSCManager");
        }

        // NOTE: the path we register for the service must use the 'SystemRoot' directive.  Using C:/Windows
        // will cause the service to fail to start with ERROR_PATH_NOT_FOUND, as the kernel is not aware of
        // drive mount letters.
        hService = ::CreateService(hSCM, L"WindscribeSplitTunnel", L"Windscribe Split Tunnel Callout Driver",
                                   SERVICE_START | DELETE | SERVICE_STOP, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                                   SERVICE_ERROR_NORMAL, L"\\SystemRoot\\system32\\DRIVERS\\WindscribeSplitTunnel.sys",
                                   L"NDIS", NULL, L"TCPIP", NULL, NULL);
        if (hService == NULL) {
            throw system_error(::GetLastError(), system_category(), "InstallSplitTunnel CreateService");
        }
    }
    catch (system_error& ex) {
        Log::instance().out(ex.what());
        result = -1;
    }

    if (hService != NULL) {
        ::CloseServiceHandle(hService);
    }

    if (hSCM != NULL) {
        ::CloseServiceHandle(hSCM);
    }

    return result;
}
