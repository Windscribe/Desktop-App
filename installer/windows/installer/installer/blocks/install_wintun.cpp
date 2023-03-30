#include "install_wintun.h"

#include "fixadaptername.h"
#include "../settings.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/process1.h"

using namespace std;

InstallWinTun::InstallWinTun(double weight) : IInstallBlock(weight, L"wintun", false)
{
}

int InstallWinTun::executeStep()
{
	unsigned long resultCode;
	wstring wintunInstallPath = Path::AddBackslash(Settings::instance().getPath()) + L"wintun\\tapinstall.exe";
	Process process;
	if (!process.InstExec(wintunInstallPath, L"install windtun420.inf windtun420", L"", ewWaitUntilTerminated, SW_HIDE, resultCode))
	{
		Log::instance().out("An error occurred installing the wintun device driver: %lu", resultCode);
		lastError_ = L"An error occurred installing the wintun device driver: " + std::to_wstring(resultCode);
		return -1;
	}
	else
	{
		Log::instance().out("Command for install wintun executed successfully");
	}

	if (resultCode != 0)
	{
		Log::instance().out("An error occurred installing the wintun device driver: %lu", resultCode);
		lastError_ = L"An error occurred installing the wintun device driver: " + std::to_wstring(resultCode);
		
		Process process;
		process.InstExec(wintunInstallPath, L"remove windtun420", L"", ewWaitUntilTerminated, SW_HIDE, resultCode);
		return -1;
	}

    if (!FixAdapterName::applyFix(L"windtun420"))
        Log::instance().out("Failed to fix \"%s\" adapter name", "windtun420");

	return 100;
}
