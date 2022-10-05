#include "service.h"

#include "../settings.h"
#include "../../../utils/path.h"
#include "../../../utils/process1.h"
#include "../../../utils/registry.h"

using namespace std;

Service::Service(double weight) : IInstallBlock(weight, L"Service")
{
}

int Service::executeStep()
{
	installWindscribeService();
	fixUnquotedServicePath();
	return 100;
}

unsigned long Service::installWindscribeService()
{
    const std::wstring& installPath = Settings::instance().getPath();
    wstring ServicePath = L"\"" + Path::AddBackslash(installPath) + L"WindscribeService.exe" + L"\"";

    unsigned long ResultCode;
    Process process;
	process.InstExec(L"sc", L"create WindscribeService binPath= " + ServicePath + L" start= auto", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);
	process.InstExec(L"sc", L"description WindscribeService \"Manages the firewall and controls the VPN tunnel\"", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);
	process.InstExec(Path::AddBackslash(installPath) + L"subinacl", L"/SERVICE WindscribeService /grant=S-1-5-11=STO", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);

	return ResultCode;
}


void Service::fixUnquotedServicePath()
{
	wstring ImagePath;

	if (Registry::RegQueryStringValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\WindscribeService", L"ImagePath", ImagePath)==true)
	{
		if ((ImagePath.length() > 0) && (ImagePath[0] != '"'))
		{
			ImagePath = L"\"" + ImagePath + L"\"";
			Registry::RegWriteStringValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\WindscribeService", L"ImagePath", ImagePath);
		}
	}
}
