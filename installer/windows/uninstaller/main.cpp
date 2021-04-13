#include "uninstall.h"
#include <string>
#include <cwctype>

using namespace std;

static std::thread uninstal_thread;


bool compareChar(const wchar_t & c1, const wchar_t & c2)
{
	if (c1 == c2)
		return true;
	else if (std::towupper(c1) == std::towupper(c2))
		return true;
	return false;
}

// Case Insensitive String Comparision
bool caseInsensitiveStringCompare(const std::wstring & str1, const std::wstring &str2)
{
	return ((str1.size() == str2.size()) &&
		std::equal(str1.begin(), str1.end(), str2.begin(), &compareChar));
}


// /SECONDPHASE="C:\Program Files (x86)\Windscribe\uninstall.exe" /FIRSTPHASEWND=0xE90448 /VERYSILENT=1
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpszCmdParam);

	Uninstaller uninstaller;

	bool isSecondPhase = false;
	bool isSilent = false;
	int nArgs;

	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	std::wstring uninstExeFile;

	if (szArglist == nullptr)
	{
		return 0;
	}
	else
	{
		for (int i = 0; i < nArgs; i++)
		{
			wstring str = szArglist[i];

			if (i == 0)
			{
				uninstExeFile = str;
				continue;
			}

			wstring AName, AValue;
			if (caseInsensitiveStringCompare(str, L"/VERYSILENT"))
			{
				isSilent = true;
			}
			else if (( str != L"") && (str[0] == '/'))
			{
				unsigned int p =  str.find(L"=");

				if (p != std::wstring::npos)
				{
					AName = str.substr(0, p);
					AValue = str.substr(p+1);

					if (caseInsensitiveStringCompare(AName, L"/SECONDPHASE"))
					{
						isSecondPhase = true;
						uninstaller.setUninstExeFile(AValue, false);
					}
					else if(caseInsensitiveStringCompare(AName, L"/FIRSTPHASEWND"))
					{
						wstring str1 = wstring(AValue.begin(),AValue.end());
						wchar_t *end;
						uninstaller.setFirstPhaseWnd(reinterpret_cast<HWND>(wcstol(str1.c_str(), &end, 16)));
					}
					else if(AName==L"/VERYSILENT")
					{
						isSilent = true;
					}
				}
			}
			
		}
	}

	LocalFree(szArglist);

	uninstaller.setSilent(isSilent);

	MSG msg;
	if (!isSecondPhase)
	{
		uninstaller.setUninstExeFile(uninstExeFile, true);
		return uninstaller.RunFirstPhase(hInstance,lpszCmdParam);
	}
	else
	{
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
				msg.wParam = 0;
			}
			else
			{
				uninstall_progress = new UninstallProgress;

				bool ret = uninstall_progress->registerClass(hInstance,nCmdShow,L"Uninstall\\ - " + ApplicationInfo::instance().getName(), L"Windscribe_uninstaller");

				if(ret == false)
				{
					delete uninstall_progress;
				}
				else
				{
					uninstal_thread = std::thread(uninstaller.RunSecondPhase,uninstall_progress->GethHwndMainWindow());
					//uninstal_thread.detach();

					// Main message loop:
					while (GetMessage(&msg, nullptr, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					uninstal_thread.join();
				}
			}
		 }
		 else
		 {
			uninstaller.RunSecondPhase(nullptr);
		 }
    }

	Log::instance().out(L"WinMain finished");
	return static_cast<int>(msg.wParam);
}

