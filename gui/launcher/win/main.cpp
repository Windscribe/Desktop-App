#include <windows.h>
#include <shlwapi.h>
#include <processthreadsapi.h>
#include <winuser.h>

BOOL isElevated();

int main()
{
    STARTUPINFO cif;
    ZeroMemory(&cif,sizeof(STARTUPINFO));
    PROCESS_INFORMATION pi;
    WCHAR szPath[MAX_PATH];

    GetModuleFileName(NULL, szPath, MAX_PATH);
    PathRemoveFileSpec(szPath);
    PathAddBackslash(szPath);
    wcscat(szPath, L"Windscribe.exe");

    if (CreateProcess(szPath, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &cif, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return 0;

}

BOOL isElevated() 
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
    {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) 
        {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) 
    {
        CloseHandle(hToken);
    }
    return fRet;
}
