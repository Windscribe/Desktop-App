#include "install_service_command.h"

#include <system_error>

#include "servicecontrolmanager.h"

using namespace std;

InstallServiceCommand::InstallServiceCommand(Logger *logger, const wchar_t *servicePath) :
    BasicCommand(logger), servicePath_(servicePath)
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

        const wstring serviceName(L"WindscribeService");
        if (scm.isServiceInstalled(serviceName.c_str())) {
            logger_->outStr(L"Remove previous instance of Windscribe Service\n");
            scm.deleteService(serviceName.c_str());
        }

        if (!servicePath_.starts_with(L'\"')) {
            servicePath_.insert(0, L"\"");
        }

        if (!servicePath_.ends_with(L'\"')) {
            servicePath_.append(L"\"");
        }

        scm.installService(serviceName.c_str(), servicePath_.c_str(),
                           L"Windscribe Service", L"Manages the firewall and controls the VPN tunnel",
                           SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, L"Nsi\0TcpIp\0", true);

        scm.setServiceSIDType(SERVICE_SID_TYPE_UNRESTRICTED);
    }
    catch (system_error& ex) {
        logger_->outStr(L"WARNING: failed to reinstall the Windscribe service - %s\n", ex.what());
    }
}
