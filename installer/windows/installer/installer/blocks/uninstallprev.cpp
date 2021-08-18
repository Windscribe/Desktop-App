#include "uninstallprev.h"
#include "../../../Utils/applicationinfo.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"

const std::wstring wmActivateGui = L"WindscribeAppActivate";


UninstallPrev::UninstallPrev(double weight) : IInstallBlock(weight, L"UninstallPrev"), state_(0)
{

}

int UninstallPrev::executeStep()
{
	if (state_ == 0)
	{
		const std::wstring classNameIcon = L"Qt5QWindowIcon";
		const std::wstring wsGuiIcon = L"Windscribe";
		HWND hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
		if (hwnd)
		{
			Log::instance().out(L"Windscribe is running - sending close");
			// SendMessage WM_CLOSE will only work on an active window
			UINT dwActivateMessage = RegisterWindowMessage(wmActivateGui.c_str());
			PostMessage(hwnd, dwActivateMessage, 0, 0);
		}
		while (hwnd)
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
			Sleep(10);
		}
		Sleep(1000);
		state_++;
		return 30;
	}
	else if (state_ == 1)
	{
		if (services.serviceExists(L"WindscribeService"))
		{
			services.simpleStopService(L"WindscribeService", true, true);
		}
		state_++;
		return 60;
	}
	else if (state_ == 2)
	{
		std::wstring uninstallString = getUninstallString();
		if (!uninstallString.empty())
		{
			// prevent uninstaller from causing browser "uninstall" popup
			Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe", L"userId", L"");
			Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"userId", L"");

			uninstallOldVersion(uninstallString);
		}
		return 100;
	}
	return 100;
}

std::wstring UninstallPrev::getUninstallString()
{
	std::wstring sUnInstallString;
	std::wstring sUnInstPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";

	if (!Registry::RegQueryStringValue(HKEY_LOCAL_MACHINE, sUnInstPath.c_str(), L"UninstallString", sUnInstallString))
	{
		Registry::RegQueryStringValue(HKEY_CURRENT_USER, sUnInstPath.c_str(), L"UninstallString", sUnInstallString);
	}
	return sUnInstallString;
}

// Return Values:
// -1 - error
// 0 - successfully executed
int UninstallPrev::uninstallOldVersion(const std::wstring &uninstallString)
{
	// get the uninstall string of the old app
	const std::wstring sUnInstallString = removeQuotes(uninstallString);

	unsigned long ResultCode;
	Process process;
	if (!process.InstExec(sUnInstallString, L"/VERYSILENT", L"", ewWaitUntilTerminated, SW_HIDE, ResultCode))
	{
		return -1;
	}

	// uninstall process can return second phase processId, which we need wait to finish
	if (ResultCode != 0)
	{
		HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, ResultCode);
		if (processHandle != NULL)
		{
			WaitForSingleObject(processHandle, INFINITE);
			CloseHandle(processHandle);
		}
	}

	return 0;
}

std::wstring UninstallPrev::removeQuotes(const std::wstring &str)
{
	std::wstring str1 = str;

	while ((!str1.empty()) && (str1[0] == '"' || str1[0] == '\''))
	{
		str1.erase(0, 1);
	}

	while ((!str1.empty()) && (str1[str1.length() - 1] == '"' || str1[str1.length() - 1] == '\''))
	{
		str1.erase(str1.length() - 1, 1);
	}

	return str1;
}


