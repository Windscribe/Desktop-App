#include "service.h"

#include "../settings.h"
#include "../../../utils/directory.h"
#include "../../../utils/logger.h"
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

void Service::installWindscribeService()
{
	const wstring& installPath = Path::AddBackslash(Settings::instance().getPath());
    wstring ServicePath = L"\"" + installPath + L"WindscribeService.exe" + L"\"";

	wstring sc = Path::AddBackslash(Directory::GetSystemDir()) + wstring(L"sc.exe");
	executeProcess(sc, L"create WindscribeService binPath= " + ServicePath + L" start= auto");
	executeProcess(sc, L"description WindscribeService \"Manages the firewall and controls the VPN tunnel\"");

	wstring subinacl = installPath + wstring(L"subinacl.exe");
	executeProcess(subinacl, L"/SERVICE WindscribeService /grant=S-1-5-11=STO");
}

void Service::fixUnquotedServicePath()
{
	wstring ImagePath;

	if (Registry::RegQueryStringValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\WindscribeService", L"ImagePath", ImagePath))
	{
		if ((ImagePath.length() > 0) && (ImagePath[0] != '"'))
		{
			ImagePath = L"\"" + ImagePath + L"\"";
			Registry::RegWriteStringValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\WindscribeService", L"ImagePath", ImagePath);
		}
	}
}

void Service::executeProcess(const wstring& process, const wstring& cmdLine)
{
	auto result = Process::InstExec(process, cmdLine, 30 * 1000, SW_HIDE);

	if (!result.has_value()) {
		Log::instance().out("Service install stage - an error was encountered launching %ls or while monitoring its progress.", process.c_str());
	}
	else if (result.value() == WAIT_TIMEOUT) {
		Log::instance().out("Service install stage - %ls timed out.", process.c_str());
	}
	else if (result.value() != NO_ERROR) {
		Log::instance().out("Service install stage - %ls returned a failure code (%lu).", process.c_str(), result.value());
	}
}
