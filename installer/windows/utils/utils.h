#pragma once

#include <windows.h>

#include <optional>
#include <string>

namespace Utils
{

// If the app is running, retrieve its main window handle.
HWND appMainWindowHandle();

bool deleteFileOnReboot(const std::wstring &file);
std::wstring desktopFolder();
std::wstring getExePath();
std::wstring getSystemDir();
std::wstring programFilesFolder();
bool isInstallerPathInProgramFilesX86(const std::wstring &installerPath);
std::wstring startMenuProgramsFolder();

// Run appName with the given commandLine and showWindowFlags.  Wait timeoutMS milliseconds for
// the process to complete.  Use timeoutMS=INFINITE to wait forever and timeoutMS=0 to not wait.
// Returns std::nullopt if there was a failure creating the process, waiting for it to complete,
// or obtaining its exit code.  Returns the process exit code or WAIT_TIMEOUT otherwise.
std::optional<DWORD> instExec(const std::wstring &appName, const std::wstring &commandLine,
                              DWORD timeoutMS, WORD showWindowFlags,
                              const std::wstring &currentFolder = std::wstring(),
                              DWORD *lastError = nullptr);

}
