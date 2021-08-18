#include "install_splittunnel.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"
#include "../../../Utils/path.h"
#include "../../../Utils/logger.h"
#include <setupapi.h>

#pragma comment(lib, "Setupapi.lib")

using namespace std;

InstallSplitTunnel::InstallSplitTunnel(const std::wstring &installPath, double weight, HWND hwnd) : IInstallBlock(weight, L"splittunnel")
{
	installPath_ = installPath;
	hwnd_ = hwnd;
}

int InstallSplitTunnel::executeStep()
{
	PVOID oldValue;
	BOOL isWowDisabled = Wow64DisableWow64FsRedirection(&oldValue);		// need for correct execute 64-bit driver install from 32-bit installer process

	wstring installCmd = L"DefaultInstall 132 ";
	installCmd += Path::AddBackslash(installPath_) + L"splittunnel\\windscribesplittunnel.inf";
	InstallHinfSection(hwnd_, NULL, installCmd.c_str(), NULL);

	if (isWowDisabled)
	{
		Wow64RevertWow64FsRedirection(oldValue);
	}

	return 100;
}
