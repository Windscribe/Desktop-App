#include "copy_and_run.h"
#include "../Utils/directories_of_a_windows.h"
#include "../Utils/redirection.h"
#include "../Utils/logger.h"

namespace
{
	std::wstring intToBase32(LONG number)
	{
		const wchar_t *table = L"0123456789ABCDEFGHIJKLMNOPQRSTUV";

		std::wstring result = L"";
		for (int I = 0; I <= 4; I++)
		{
			result.insert(0, std::wstring(&table[number & 31], 1));
			number = number >> 5;
		}

		return result;
	}

	// Returns True if it overwrote an existing file.
	bool generateNonRandomUniqueFilename(std::wstring path1, std::wstring &outFilename)
	{
		Redirection redirection;
		long Rand, RandOrig;
		HANDLE F;
		bool Success;
		std::wstring FN;
		bool Result;

		Path path;
		Directory dir;

		path1 = path.AddBackslash(path1);
		RandOrig = 0x123456;
		Rand = RandOrig;
		Success = false;
		Result = false;
		while (1)
		{
			Rand++;
			if (Rand > 0x1FFFFFF) Rand = 0;
			if (Rand == RandOrig)
			{
				//  practically impossible to go through 33 million possibilities, but check "just in case"...
				return false;
			}
			FN = path1 + L"_iu" + intToBase32(Rand) + L".tmp";
			if (dir.DirExists(FN)) continue;
			Success = true;
			Result = redirection.NewFileExists(FN);

			if (Result == true)
			{
				F = CreateFile(FN.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				Success = (F != INVALID_HANDLE_VALUE);
				if (Success == true)
				{
					CloseHandle(F);
				}
			}

			if (Success == true)
			{
				break;
			}
		}

		outFilename = FN;
		return true;
	}

	HANDLE exec(const std::wstring &filename, const std::wstring &workingDir, const std::wstring &pars, DWORD &outProcessId)
	{
		std::wstring cmdLine;
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION processInfo;

		cmdLine = L"\"" + filename + L"\" " + pars;

		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		if (CreateProcess(nullptr, const_cast<wchar_t*>(cmdLine.c_str()), nullptr, nullptr, false, 0, nullptr, workingDir.c_str(), &startupInfo, &processInfo) == false)
		{
			outProcessId = 0;
			return NULL;
		}

		outProcessId = processInfo.dwProcessId;
		CloseHandle(processInfo.hThread);

		return processInfo.hProcess;
	}
}

namespace CopyAndRun
{
	int runFirstPhase(const std::wstring &uninstExeFile, LPSTR lpszCmdParam)
	{
		DirectoriesOfAWindows dir_win;
		std::wstring tempFile;
		std::wstring tempWinDir = dir_win.GetTempDir();

		//{ Copy self to TEMP directory with a name like _iu14D2N.tmp. The
		//  actual uninstallation process must be done from somewhere outside
		//  the application directory since EXE's can't delete themselves while
		//  they are running. }
		if (!generateNonRandomUniqueFilename(tempWinDir, tempFile))
		{
			return 0;
		}
		if (!CopyFile(uninstExeFile.c_str(), tempFile.c_str(), false))
		{
			return 0;
		}
		// { Don't want any attribute like read-only transferred }
		SetFileAttributes(tempFile.c_str(), FILE_ATTRIBUTE_NORMAL);

		//    { Execute the copy of itself ("second phase") }
		const int BUF_SIZE = MAX_PATH * 2;
		wchar_t buffer[BUF_SIZE];
		swprintf(buffer, BUF_SIZE, L"/SECONDPHASE=\"%s\" ", uninstExeFile.c_str());
		std::string str = std::string(lpszCmdParam);
		std::wstring str1 = std::wstring(str.begin(), str.end());
		DWORD dwProcessId = 0;
		Log::instance().out(L"Start process: %s", tempWinDir.c_str());
		HANDLE processHandle = exec(tempFile, tempWinDir, std::wstring(buffer) + str1, dwProcessId);
		CloseHandle(processHandle);
		return dwProcessId;
	}
}