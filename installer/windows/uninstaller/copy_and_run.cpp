#include "copy_and_run.h"

#include <shlwapi.h>
#include <sstream>

#include "../Utils/directories_of_a_windows.h"
#include "../Utils/logger.h"
#include "../Utils/redirection.h"

#pragma comment(lib, "Shlwapi.lib")

namespace
{
    std::wstring intToBase32(LONG number)
    {
        const wchar_t* table = L"0123456789ABCDEFGHIJKLMNOPQRSTUV";

        std::wstring result = L"";
        for (int I = 0; I <= 4; I++)
        {
            result.insert(0, std::wstring(&table[number & 31], 1));
            number = number >> 5;
        }

        return result;
    }

    bool generateNonRandomUniqueFilename(std::wstring path1, std::wstring& outFilename)
    {
        path1 = Path::AddBackslash(path1);

        long RandOrig = 0x123456;
        long Rand = RandOrig;

        while (true)
        {
            Rand++;
            if (Rand > 0x1FFFFFF) Rand = 0;
            if (Rand == RandOrig) {
                //  practically impossible to go through 33 million possibilities, but check "just in case"...
                return false;
            }

            std::wstring FN = path1 + L"_iu" + intToBase32(Rand) + L".tmp";
            if (::PathFileExists(FN.c_str()) == FALSE) {
                outFilename = FN;
                break;
            }
        }

        return true;
    }

    DWORD exec(const std::wstring& filename, const std::wstring& workingDir, const std::wstring& pars)
    {
        std::wstring cmdLine = L"\"" + filename + L"\" " + pars;

        STARTUPINFO startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo;
        BOOL result = ::CreateProcess(nullptr, const_cast<wchar_t*>(cmdLine.c_str()), nullptr, nullptr, false, 0, nullptr,
                                      workingDir.c_str(), &startupInfo, &processInfo);
        if (result == FALSE) {
            Log::instance().out("CopyAndRun::exec - CreateProcess(%s) failed (%lu)", cmdLine.c_str(), ::GetLastError());
            return 0;
        }

        ::CloseHandle(processInfo.hThread);
        ::CloseHandle(processInfo.hProcess);

        return processInfo.dwProcessId;
    }
}

namespace CopyAndRun
{
    int runFirstPhase(const std::wstring& uninstExeFile, LPSTR lpszCmdParam)
    {
        DirectoriesOfAWindows dir_win;
        std::wstring tempFile;
        std::wstring tempWinDir = dir_win.GetTempDir();

        // Copy self to TEMP directory with a name like _iu14D2N.tmp. The
        // actual uninstallation process must be done from somewhere outside
        // the application directory since EXE's can't delete themselves while
        // they are running.
        if (!generateNonRandomUniqueFilename(tempWinDir, tempFile)) {
            Log::instance().out("CopyAndRun::runFirstPhase generateNonRandomUniqueFilename failed");
            return 0;
        }

        // We should have generated a unique filename that does not exist in the target folder, 
        // thus CopyFile should fail if the file already exists.
        if (!CopyFile(uninstExeFile.c_str(), tempFile.c_str(), TRUE)) {
            Log::instance().out("CopyAndRun::runFirstPhase failed to copy [%s] to [%s] (%lu)", uninstExeFile.c_str(), tempFile.c_str(), ::GetLastError());
            return 0;
        }

        // Don't want any attribute like read-only transferred
        ::SetFileAttributes(tempFile.c_str(), FILE_ATTRIBUTE_NORMAL);

        // Execute the copy of itself ("second phase")
        std::wstring pars;
        {
            std::wostringstream stream;
            stream << L"/SECONDPHASE=\"" << uninstExeFile << L"\" " << lpszCmdParam;
            pars = stream.str();
        }

        Log::instance().out(L"Start process: %s %s", tempFile.c_str(), pars.c_str());
        return exec(tempFile, tempWinDir, pars);
    }
}