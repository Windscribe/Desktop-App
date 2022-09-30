#include "install_tap.h"

#include "fixadaptername.h"
#include "../settings.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/process1.h"

using namespace std;

InstallTap::InstallTap(double weight) : IInstallBlock(weight, L"Tap", false)
{
}

int InstallTap::executeStep()
{
	unsigned long resultCode;
	wstring tapInstallPath = Path::AddBackslash(Settings::instance().getPath()) + L"tap\\tapinstall.exe";
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
