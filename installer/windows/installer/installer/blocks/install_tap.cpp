#include "install_tap.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"
#include "../../../Utils/path.h"
#include "../../../Utils/logger.h"
#include "fixadaptername.h"

using namespace std;

InstallTap::InstallTap(const std::wstring &installPath, double weight) : IInstallBlock(weight, L"Tap", false)
{
	installPath_ = installPath;
}

int InstallTap::executeStep()
{
	unsigned long resultCode;
	wstring tapInstallPath = Path::AddBackslash(installPath_) + L"tap\\tapinstall.exe";
	Process process;
	if (!process.InstExec(tapInstallPath, L"install OemVista.inf tapwindscribe0901", L"", ewWaitUntilTerminated, SW_HIDE, resultCode))
	{
		Log::instance().out("An error occurred installing the TAP device driver: %lu", resultCode);
		lastError_ = L"An error occurred installing the TAP device driver: " + std::to_wstring(resultCode);
		return -1;
	}
	else
	{
		Log::instance().out("Command for install tap executed successfully");
	}

	if (resultCode != 0)
	{
		Log::instance().out("An error occurred installing the TAP device driver: %lu", resultCode);
		lastError_ = L"An error occurred installing the TAP device driver: " + std::to_wstring(resultCode);

		Process process;
		process.InstExec(tapInstallPath, L"remove tapwindscribe0901", L"", ewWaitUntilTerminated, SW_HIDE, resultCode);
		return -1;
	}

    if (!FixAdapterName::applyFix(L"tapwindscribe0901"))
        Log::instance().out("Failed to fix \"%s\" adapter name", "tapwindscribe0901");

	return 100;
}
