#include "uninstall.h"
#include "arguments_parser.h"
#include "copy_and_run.h"

static std::thread uninstal_thread;


// Let's compare where the uninstall file is located with where the program was originally installed
// If they are different then we do not uninstall, to avoid deleting 3-rd party files
bool isRightInstallation(const std::wstring &uninstExeFile)
{
	HKEY rootKey = HKEY_LOCAL_MACHINE;
	std::wstring subkeyName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + ApplicationInfo::instance().getId() + L"_is1";

	bool bRet = false;
	HKEY hKey;
	if (Registry::RegOpenKeyExView(rvDefault, rootKey, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey) == ERROR_SUCCESS)
	{
		std::wstring path;
		if (Registry::RegQueryStringValue1(hKey, L"InstallLocation", path))
		{
			bRet = (uninstExeFile.rfind(path, 0) == 0);   // uninstExeFile string starts with path?
		}
		RegCloseKey(hKey);
	}
	return bRet;
}

// /SECONDPHASE="C:\Program Files (x86)\Windscribe\uninstall.exe" /VERYSILENT
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpszCmdParam);

	ArgumentsParser argumentParser;
	if (!argumentParser.parse())
	{
		Log::instance().out(L"ArgumentsParser failed");
		return 0;
	}

	bool isSilent = argumentParser.getArg(L"/VERYSILENT", nullptr);
	std::wstring uninstExeFile;
	bool isSecondPhase = argumentParser.getArg(L"/SECONDPHASE", &uninstExeFile);
	if (!isSecondPhase)
	{
		uninstExeFile = argumentParser.getExecutablePath();
	}


	Log::instance().init(false);

	if (!isSecondPhase)
	{
		Log::instance().out(L"First phase started");
		if (!isRightInstallation(uninstExeFile))
		{
			MessageBox(
				nullptr,
				reinterpret_cast<LPCWSTR>(L"Uninstall can't be done. The installation directory does not match the removal directory."),
				reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
				MB_OK);
			return 0;
		}
		else
		{
			return CopyAndRun::runFirstPhase(uninstExeFile, lpszCmdParam);
		}
	}
	else
	{
		Log::instance().out(L"Second phase started");
		Uninstaller uninstaller;
		uninstaller.setSilent(isSilent);
		uninstaller.setUninstExeFile(uninstExeFile, false);

		int msgboxID = IDYES;

		 if (!isSilent)
		 {
			msgboxID =  MessageBox(
				   nullptr,
				   reinterpret_cast<LPCWSTR>(L"Are you sure you want to completely remove Windscribe and all of its components?"),
				   reinterpret_cast<LPCWSTR>(L"Windscribe Uninstall"),
				   MB_ICONQUESTION | MB_YESNO);

			if (msgboxID == IDNO)
			{
				return 0;
			}
			else
			{
				uninstaller.RunSecondPhase();
			}
		 }
		 else
		 {
			uninstaller.RunSecondPhase();
		 }
    }

	Log::instance().out(L"WinMain finished");
	return 0;
}

