#include "uninstallprev.h"
#include "../../../Utils/applicationinfo.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"
#include "../../../../../common/utils/wsscopeguard.h"

const std::wstring wmActivateGui = L"WindscribeAppActivate";


UninstallPrev::UninstallPrev(bool isFactoryReset, double weight) : IInstallBlock(weight, L"UninstallPrev"),
	state_(0), isFactoryReset_(isFactoryReset)
{
}

int UninstallPrev::executeStep()
{
    if (state_ == 0)
    {
        std::wstring classNameIcon = L"Qt624QWindowIcon";
        const std::wstring wsGuiIcon = L"Windscribe";
        HWND hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
        if (hwnd == NULL)
        {
            // Check if the old Qt 5.12 app is running.
            classNameIcon = L"Qt5QWindowIcon";
            hwnd = FindWindow(classNameIcon.c_str(), wsGuiIcon.c_str());
        }
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
			if (isFactoryReset_)
			{
				doFactoryReset();
			}
			else
			{
				// prevent uninstaller from causing browser "uninstall" popup
				Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe", L"userId", L"");
				Registry::RegWriteStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"userId", L"");
			}
			uninstallOldVersion(uninstallString);
        }
        return 100;
    }
    return 100;
}

std::wstring UninstallPrev::getUninstallString()
{
    HKEY hKey = NULL;
    auto exitGuard = wsl::wsScopeGuard([&]
    {
        if (hKey != NULL) {
            ::RegCloseKey(hKey);
        }
    });

    // Check the 64-bit hive first.  If the entry is not found, check if it exists in the 32-bit hive
    // from an old 32-bit install.
    std::wstring subkeyName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";
    auto status = Registry::RegOpenKeyExView(rvDefault, HKEY_LOCAL_MACHINE, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey);

    if (status != ERROR_SUCCESS) {
        status = Registry::RegOpenKeyExView(rv32Bit, HKEY_LOCAL_MACHINE, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey);
    }

    std::wstring uninstallString;
    if (status == ERROR_SUCCESS) {
        Registry::RegQueryStringValue1(hKey, L"UninstallString", uninstallString);
    }

    return uninstallString;
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

void UninstallPrev::doFactoryReset()
{
	// Delete preferences
	Registry::RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe");
	Registry::RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2");

	// Delete logs and other data
	// NB: Does not delete any files in use, such as the uninstaller log
	wchar_t* localAppData = nullptr;

	if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData) == S_OK)
	{
		std::wstring deletePath(localAppData);
		deletePath.append(L"\\windscribe_extra.conf");
		DeleteFile(deletePath.c_str());

		deletePath = localAppData;
		CoTaskMemFree(static_cast<void*>(localAppData));
		deletePath.append(L"\\Windscribe\\Windscribe2\\*");

		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA ffd;

		hFind = FindFirstFile(deletePath.c_str(), &ffd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				std::wstring fullPath(deletePath);
				fullPath.erase(fullPath.size()-1); // erase '*'
				fullPath.append(ffd.cFileName);
				DeleteFile(fullPath.c_str());
			}
			while (FindNextFile(hFind, &ffd) != 0);
			FindClose(hFind);
		}

		DeleteFile(deletePath.c_str());
	}
}