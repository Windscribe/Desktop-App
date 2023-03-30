/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#include "elevate.h"
#include "logger.h"

#include <Windows.h>
#include <TlHelp32.h>

_Return_type_success_(return != FALSE) BOOL ElevateToSystem(void)
{
    HANDLE CurrentProcessToken, ThreadToken, ProcessSnapshot, WinlogonProcess, WinlogonToken, DuplicatedToken;
    PROCESSENTRY32W ProcessEntry = { .dwSize = sizeof(PROCESSENTRY32W) };
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
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to create SID");
        goto cleanup;
    }
    Ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &CurrentProcessToken);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to open process token");
        goto cleanup;
    }
    Ret =
        GetTokenInformation(CurrentProcessToken, TokenUser, &TokenUserBuffer, sizeof(TokenUserBuffer), &RequiredBytes);
    LastError = GetLastError();
    CloseHandle(CurrentProcessToken);
    if (!Ret)
    {
        LOG_ERROR(L"Failed to get token information", LastError);
        goto cleanup;
    }
    if (EqualSid(TokenUserBuffer.MaybeLocalSystem.User.Sid, LocalSystemSid))
        return TRUE;
    Ret = LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &Privileges.Privileges[0].Luid);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to lookup privilege value");
        goto cleanup;
    }
    ProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (ProcessSnapshot == INVALID_HANDLE_VALUE)
    {
        LastError = LOG_LAST_ERROR(L"Failed to create toolhelp snapshot");
        goto cleanup;
    }
    for (Ret = Process32FirstW(ProcessSnapshot, &ProcessEntry); Ret;
         Ret = Process32NextW(ProcessSnapshot, &ProcessEntry))
    {
        if (_wcsicmp(ProcessEntry.szExeFile, L"winlogon.exe"))
            continue;
        RevertToSelf();
        Ret = ImpersonateSelf(SecurityImpersonation);
        if (!Ret)
        {
            LastError = GetLastError();
            continue;
        }
        Ret = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, FALSE, &ThreadToken);
        if (!Ret)
        {
            LastError = GetLastError();
            continue;
        }
        Ret = AdjustTokenPrivileges(ThreadToken, FALSE, &Privileges, sizeof(Privileges), NULL, NULL);
        LastError = GetLastError();
        CloseHandle(ThreadToken);
        if (!Ret)
            continue;

        WinlogonProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ProcessEntry.th32ProcessID);
        if (!WinlogonProcess)
        {
            LastError = GetLastError();
            continue;
        }
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
        if (!EqualSid(TokenUserBuffer.MaybeLocalSystem.User.Sid, LocalSystemSid))
        {
            SetLastError(ERROR_ACCESS_DENIED);
            goto next;
        }
        if (!SetThreadToken(NULL, DuplicatedToken))
            goto next;
        CloseHandle(DuplicatedToken);
        CloseHandle(ProcessSnapshot);
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

_Return_type_success_(return != NULL) HANDLE GetPrimarySystemTokenFromThread(void)
{
    HANDLE CurrentToken, DuplicatedToken;
    BOOL Ret;
    DWORD LastError;
    TOKEN_PRIVILEGES Privileges = { .PrivilegeCount = 1, .Privileges = { { .Attributes = SE_PRIVILEGE_ENABLED } } };
    CHAR LocalSystemSid[0x400];
    DWORD RequiredBytes = sizeof(LocalSystemSid);
    struct
    {
        TOKEN_USER MaybeLocalSystem;
        CHAR LargeEnoughForLocalSystem[0x400];
    } TokenUserBuffer;

    Ret = CreateWellKnownSid(WinLocalSystemSid, NULL, &LocalSystemSid, &RequiredBytes);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to create SID");
        return NULL;
    }
    Ret = OpenThreadToken(
        GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_DUPLICATE, FALSE, &CurrentToken);
    if (!Ret && GetLastError() == ERROR_NO_TOKEN)
        Ret = OpenProcessToken(
            GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_DUPLICATE, &CurrentToken);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to open token");
        return NULL;
    }
    Ret = GetTokenInformation(CurrentToken, TokenUser, &TokenUserBuffer, sizeof(TokenUserBuffer), &RequiredBytes);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to get token information");
        goto cleanup;
    }
    if (!EqualSid(TokenUserBuffer.MaybeLocalSystem.User.Sid, LocalSystemSid))
    {
        LOG(WINTUN_LOG_ERR, L"Not SYSTEM");
        LastError = ERROR_ACCESS_DENIED;
        goto cleanup;
    }
    Ret = LookupPrivilegeValueW(NULL, SE_ASSIGNPRIMARYTOKEN_NAME, &Privileges.Privileges[0].Luid);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to lookup privilege value");
        goto cleanup;
    }
    Ret = AdjustTokenPrivileges(CurrentToken, FALSE, &Privileges, sizeof(Privileges), NULL, NULL);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to adjust token privileges");
        goto cleanup;
    }
    Ret = DuplicateTokenEx(
        CurrentToken,
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
        NULL,
        SecurityImpersonation,
        TokenPrimary,
        &DuplicatedToken);
    if (!Ret)
    {
        LastError = LOG_LAST_ERROR(L"Failed to duplicate token");
        goto cleanup;
    }
    CloseHandle(CurrentToken);
    return DuplicatedToken;

cleanup:
    CloseHandle(CurrentToken);
    SetLastError(LastError);
    return NULL;
}
