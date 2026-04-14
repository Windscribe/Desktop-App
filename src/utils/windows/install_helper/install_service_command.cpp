#include "ws_branding.h"
#include "install_service_command.h"

#include <sstream>
#include <system_error>

#include "servicecontrolmanager.h"

using namespace std;

InstallServiceCommand::InstallServiceCommand(Logger *logger, const wstring &installDir)
    : BasicCommand(logger),
    installDir_(installDir)
{
}

InstallServiceCommand::~InstallServiceCommand()
{
}

void InstallServiceCommand::execute()
{
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_ALL_ACCESS);

        const wstring serviceName(WS_APP_IDENTIFIER_W L"Service");
        if (scm.isServiceInstalled(serviceName.c_str())) {
            logger_->outStr(L"Removing previous instance of service");
            std::error_code ec;
            if (!scm.deleteService(serviceName.c_str(), ec)) {
                // We'll still try to reinstall the service even if the delete failed/timed out.
                logger_->outStr(L"Failed to delete existing service (%d)", ec.value());
            }
        }

        wstring serviceCmdLine;
        {
            wostringstream stream;
            stream << L"\"" << installDir_ << L"\\" << WS_APP_IDENTIFIER_W << L"Service.exe\"";
            serviceCmdLine = stream.str();
        }

        scm.installService(serviceName.c_str(), serviceCmdLine.c_str(),
                           WS_PRODUCT_NAME_W L" Service", L"Manages the firewall and controls the VPN tunnel",
                           SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, L"Nsi\0TcpIp\0", true);
        scm.setServiceSIDType(SERVICE_SID_TYPE_UNRESTRICTED);
        scm.openService(serviceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService();

        logger_->outStr(L"Reinstalled service");
    }
    catch (system_error& ex) {
        logger_->outStr(L"WARNING: failed to reinstall service - %s", ex.what());
    }
}
