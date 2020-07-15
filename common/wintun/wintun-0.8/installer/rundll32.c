/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2019 WireGuard LLC. All Rights Reserved.
 */

#include "installation.h"
#include <Windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#pragma warning(disable : 4100) /* unreferenced formal parameter */

static VOID
ConsoleLogger(_In_ LOGGER_LEVEL Level, _In_ const TCHAR *LogLine)
{
    TCHAR *Template;
    switch (Level)
    {
    case LOG_INFO:
        Template = TEXT("[+] %s\n");
        break;
    case LOG_WARN:
        Template = TEXT("[-] %s\n");
        break;
    case LOG_ERR:
        Template = TEXT("[!] %s\n");
        break;
    default:
        return;
    }
    _ftprintf(stdout, Template, LogLine);
}

static BOOL ElevateToSystem(VOID)
{
    HANDLE CurrentProcessToken, ThreadToken, ProcessSnapshot, WinlogonProcess, WinlogonToken, DuplicatedToken;
    PROCESSENTRY32 ProcessEntry = { .dwSize = sizeof(PROCESSENTRY32) };
    BOOL Ret;
    DWORD LastError = ERROR_SUCCESS;
    TOKEN_PRIVILEGES Privileges = { .PrivilegeCount = 1, .Privileges = { { .Attributes = SE_PRIVILEGE_ENABLED } } };
    CHAR LocalSystemSid[0x400];
    DWORD RequiredBytes = sizeof(LocalSystemSid);
    struct
    {
        TOKEN_USER MaybeLocalSystem;
        CHAR LargeEnoughForLocalSystem[0x400];
    } TokenUserBuffer;

    Ret = CreateWellKnownSid(WinLocalSystemSid, NULL, &LocalSystemSid, &RequiredBytes);
    LastError = GetLastError();
    if (!Ret)
        goto cleanup;
    Ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &CurrentProcessToken);
    LastError = GetLastError();
    if (!Ret)
        goto cleanup;
    Ret =
        GetTokenInformation(CurrentProcessToken, TokenUser, &TokenUserBuffer, sizeof(TokenUserBuffer), &RequiredBytes);
    LastError = GetLastError();
    CloseHandle(CurrentProcessToken);
    if (!Ret)
        goto cleanup;
    if (EqualSid(TokenUserBuffer.MaybeLocalSystem.User.Sid, LocalSystemSid))
        return TRUE;
    Ret = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Privileges.Privileges[0].Luid);
    LastError = GetLastError();
    if (!Ret)
        goto cleanup;
    ProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    LastError = GetLastError();
    if (ProcessSnapshot == INVALID_HANDLE_VALUE)
        goto cleanup;
    for (Ret = Process32First(ProcessSnapshot, &ProcessEntry); Ret; Ret = Process32Next(ProcessSnapshot, &ProcessEntry))
    {
        if (_tcsicmp(ProcessEntry.szExeFile, TEXT("winlogon.exe")))
            continue;
        RevertToSelf();
        Ret = ImpersonateSelf(SecurityImpersonation);
        LastError = GetLastError();
        if (!Ret)
            continue;
        Ret = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, FALSE, &ThreadToken);
        LastError = GetLastError();
        if (!Ret)
            continue;
        Ret = AdjustTokenPrivileges(ThreadToken, FALSE, &Privileges, sizeof(Privileges), NULL, NULL);
        LastError = GetLastError();
        CloseHandle(ThreadToken);
        if (!Ret)
            continue;

        WinlogonProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ProcessEntry.th32ProcessID);
        LastError = GetLastError();
        if (!WinlogonProcess)
            continue;
        Ret = OpenProcessToken(WinlogonProcess, TOKEN_IMPERSONATE | TOKEN_DUPLICATE, &WinlogonToken);
        LastError = GetLastError();
        CloseHandle(WinlogonProcess);
        if (!Ret)
            continue;
        Ret = DuplicateToken(WinlogonToken, SecurityImpersonation, &DuplicatedToken);
        LastError = GetLastError();
        CloseHandle(WinlogonToken);
        if (!Ret)
            continue;
        if (!GetTokenInformation(DuplicatedToken, TokenUser, &TokenUserBuffer, sizeof(TokenUserBuffer), &RequiredBytes))
            goto next;
        if (SetLastError(ERROR_ACCESS_DENIED), !EqualSid(TokenUserBuffer.MaybeLocalSystem.User.Sid, LocalSystemSid))
            goto next;
        if (!SetThreadToken(NULL, DuplicatedToken))
            goto next;
        CloseHandle(DuplicatedToken);
        CloseHandle(ProcessSnapshot);
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    next:
        LastError = GetLastError();
        CloseHandle(DuplicatedToken);
    }
    RevertToSelf();
    CloseHandle(ProcessSnapshot);
cleanup:
    SetLastError(LastError);
    return FALSE;
}

static VOID
RunAsAdministrator(HWND hwnd, TCHAR *Verb, int nCmdShow)
{
    TOKEN_ELEVATION Elevation;
    DWORD Required;
    HANDLE CurrentProcessToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &CurrentProcessToken))
        return;
    BOOL Ret = GetTokenInformation(CurrentProcessToken, TokenElevation, &Elevation, sizeof(Elevation), &Required);
    CloseHandle(CurrentProcessToken);
    if (!Ret)
        return;
    if (Elevation.TokenIsElevated)
        return;
    TCHAR ProcessPath[MAX_PATH], DllPath[MAX_PATH];
    if (!GetModuleFileName(NULL, ProcessPath, _countof(ProcessPath)) ||
        !GetModuleFileName(ResourceModule, DllPath, _countof(DllPath)))
        return;
    TCHAR Params[0x1000];
    _stprintf_s(Params, _countof(Params), TEXT("\"%s\",%s"), DllPath, Verb);
    ShellExecute(hwnd, TEXT("runas"), ProcessPath, Params, NULL, nCmdShow);
    exit(0);
}

static VOID
Do(BOOL Install, BOOL ShowConsole)
{
    if (ShowConsole)
    {
        AllocConsole();
        FILE *Stream;
        freopen_s(&Stream, "CONOUT$", "w", stdout);
    }
    SetLogger(ConsoleLogger);
    ElevateToSystem();
    Install ? InstallOrUpdate() : Uninstall();
    RevertToSelf();
    _putws(TEXT("\nPress any key to close . . ."));
    (VOID) _getch();
}

VOID __stdcall InstallWintun(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    RunAsAdministrator(hwnd, TEXT(__FUNCTION__), nCmdShow);
    Do(TRUE, !!nCmdShow);
}

VOID __stdcall UninstallWintun(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    RunAsAdministrator(hwnd, TEXT(__FUNCTION__), nCmdShow);
    Do(FALSE, !!nCmdShow);
}