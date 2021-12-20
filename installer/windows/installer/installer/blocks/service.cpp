#include "service.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"
#include "../../../Utils/path.h"

using namespace std;

Service::Service(const std::wstring &installPath, double weight) : IInstallBlock(weight, L"Service")
{
	installPath_ = installPath;
}

int Service::executeStep()
{
	installWindscribeService(installPath_);
	fixUnquotedServicePath();
	return 100;
}

unsigned long Service::installWindscribeService(const wstring &path_for_installation)
{
	unsigned long ResultCode;
	wstring ServicePath;

	ServicePath = L"\"" + Path::AddBackslash(installPath_) + L"WindscribeService.exe" + L"\"";

	Process process;
	process.InstExec(L"sc", L"create WindscribeService binPath= " + ServicePath + L" start= auto", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);
	process.InstExec(L"sc", L"description WindscribeService \"Manages the firewall and controls the VPN tunnel\"", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);
	process.InstExec(Path::AddBackslash(installPath_) + L"subinacl", L"/SERVICE WindscribeService /grant=S-1-5-11=STO", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode);

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
