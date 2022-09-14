/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#include "entry.h"
#include "logger.h"
#include "registry.h"
#include <Windows.h>
#include <wchar.h>

static _Return_type_success_(return != NULL) HKEY
    OpenKeyWait(_In_ HKEY Key, _Inout_z_ WCHAR *Path, _In_ DWORD Access, _In_ ULONGLONG Deadline)
{
    DWORD LastError;
    WCHAR *PathNext = wcschr(Path, L'\\');
    if (PathNext)
        *PathNext = 0;

    HANDLE Event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!Event)
    {
        LOG_LAST_ERROR(L"Failed to create event");
        return NULL;
    }
    for (;;)
    {
        LastError = RegNotifyChangeKeyValue(Key, FALSE, REG_NOTIFY_CHANGE_NAME, Event, TRUE);
        if (LastError != ERROR_SUCCESS)
        {
            LOG_ERROR(L"Failed to setup notification", LastError);
            break;
        }

        HKEY Subkey;
        LastError = RegOpenKeyExW(Key, Path, 0, PathNext ? KEY_NOTIFY : Access, &Subkey);
        if (LastError == ERROR_SUCCESS)
        {
            if (PathNext)
            {
                HKEY KeyOut = OpenKeyWait(Subkey, PathNext + 1, Access, Deadline);
                if (KeyOut)
                {
                    RegCloseKey(Subkey);
                    CloseHandle(Event);
                    return KeyOut;
                }
                LastError = GetLastError();
                break;
            }
            else
            {
                CloseHandle(Event);
                return Subkey;
            }
        }
        if (LastError != ERROR_FILE_NOT_FOUND && LastError != ERROR_PATH_NOT_FOUND)
        {
            LOG_ERROR(L"Failed to open", LastError);
            break;
        }

        LONGLONG TimeLeft = Deadline - GetTickCount64();
        if (TimeLeft < 0)
            TimeLeft = 0;
        if (WaitForSingleObject(Event, (DWORD)TimeLeft) != WAIT_OBJECT_0)
        {
            LOG(WINTUN_LOG_ERR, L"Timeout waiting");
            break;
        }
    }
    CloseHandle(Event);
    SetLastError(LastError);
    return NULL;
}

_Return_type_success_(return != NULL) HKEY
    RegistryOpenKeyWait(_In_ HKEY Key, _In_z_ const WCHAR *Path, _In_ DWORD Access, _In_ DWORD Timeout)
{
    WCHAR Buf[MAX_REG_PATH];
    if (wcsncpy_s(Buf, _countof(Buf), Path, _TRUNCATE) == STRUNCATE)
    {
        LOG(WINTUN_LOG_ERR, L"Registry path too long");
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    return OpenKeyWait(Key, Buf, Access, GetTickCount64() + Timeout);
}

_Return_type_success_(return != FALSE) BOOL RegistryGetString(_Inout_ WCHAR **Buf, _In_ DWORD Len, _In_ DWORD ValueType)
{
    if (wcsnlen(*Buf, Len) >= Len)
    {
        /* String is missing zero-terminator. */
        WCHAR *BufZ = Alloc(((size_t)Len + 1) * sizeof(WCHAR));
        if (!BufZ)
            return FALSE;
        wmemcpy(BufZ, *Buf, Len);
        BufZ[Len] = 0;
        Free(*Buf);
        *Buf = BufZ;
    }

    if (ValueType != REG_EXPAND_SZ)
        return TRUE;

    /* ExpandEnvironmentStringsW() returns strlen on success or 0 on error. Bail out on empty input strings to
     * disambiguate. */
    if (!(*Buf)[0])
        return TRUE;

    Len = Len * 2 + 64;
    for (;;)
    {
        WCHAR *Expanded = Alloc(Len * sizeof(WCHAR));
        if (!Expanded)
            return FALSE;
        DWORD Result = ExpandEnvironmentStringsW(*Buf, Expanded, Len);
        if (!Result)
        {
            LOG_LAST_ERROR(L"Failed to expand environment variables");
            Free(Expanded);
            return FALSE;
        }
        if (Result > Len)
        {
            Free(Expanded);
            Len = Result;
            continue;
        }
        Free(*Buf);
        *Buf = Expanded;
        return TRUE;
    }
}

_Return_type_success_(return != FALSE) BOOL
    RegistryGetMultiString(_Inout_ WCHAR **Buf, _In_ DWORD Len, _In_ DWORD ValueType)
{
    if (ValueType == REG_MULTI_SZ)
    {
        for (size_t i = 0;; i += wcsnlen(*Buf + i, Len - i) + 1)
        {
            if (i > Len)
            {
                /* Missing string and list terminators. */
                WCHAR *BufZ = Alloc(((size_t)Len + 2) * sizeof(WCHAR));
                if (!BufZ)
                    return FALSE;
                wmemcpy(BufZ, *Buf, Len);
                BufZ[Len] = 0;
                BufZ[Len + 1] = 0;
                Free(*Buf);
                *Buf = BufZ;
                return TRUE;
            }
            if (i == Len)
            {
                /* Missing list terminator. */
                WCHAR *BufZ = Alloc(((size_t)Len + 1) * sizeof(WCHAR));
                if (!BufZ)
                    return FALSE;
                wmemcpy(BufZ, *Buf, Len);
                BufZ[Len] = 0;
                Free(*Buf);
                *Buf = BufZ;
                return TRUE;
            }
            if (!(*Buf)[i])
                return TRUE;
        }
    }

    /* Sanitize REG_SZ/REG_EXPAND_SZ and append a list terminator to make a multi-string. */
    if (!RegistryGetString(Buf, Len, ValueType))
        return FALSE;
    Len = (DWORD)wcslen(*Buf) + 1;
    WCHAR *BufZ = Alloc(((size_t)Len + 1) * sizeof(WCHAR));
    if (!BufZ)
        return FALSE;
    wmemcpy(BufZ, *Buf, Len);
    BufZ[Len] = 0;
    Free(*Buf);
    *Buf = BufZ;
    return TRUE;
}

static _Return_type_success_(return != NULL) void *RegistryQuery(
    _In_ HKEY Key,
    _In_opt_z_ const WCHAR *Name,
    _Out_opt_ DWORD *ValueType,
    _Inout_ DWORD *BufLen,
    _In_ BOOL Log)
{
    for (;;)
    {
        BYTE *p = Alloc(*BufLen);
        if (!p)
            return NULL;
        LSTATUS LastError = RegQueryValueExW(Key, Name, NULL, ValueType, p, BufLen);
        if (LastError == ERROR_SUCCESS)
            return p;
        Free(p);
        if (LastError != ERROR_MORE_DATA)
        {
            if (Log)
                LOG_ERROR(L"Querying value failed", LastError);
            SetLastError(LastError);
            return NULL;
        }
    }
}

_Return_type_success_(
    return != NULL) WCHAR *RegistryQueryString(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ BOOL Log)
{
    DWORD LastError, ValueType, Size = 256 * sizeof(WCHAR);
    WCHAR *Value = RegistryQuery(Key, Name, &ValueType, &Size, Log);
    if (!Value)
        return NULL;
    switch (ValueType)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        if (RegistryGetString(&Value, Size / sizeof(WCHAR), ValueType))
            return Value;
        LastError = GetLastError();
        break;
    default:
        LOG(WINTUN_LOG_ERR, L"Value is not a string");
        LastError = ERROR_INVALID_DATATYPE;
    }
    Free(Value);
    SetLastError(LastError);
    return NULL;
}

_Return_type_success_(
    return != NULL) WCHAR *RegistryQueryStringWait(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ DWORD Timeout)
{
    DWORD LastError;
    ULONGLONG Deadline = GetTickCount64() + Timeout;
    HANDLE Event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!Event)
    {
        LOG_LAST_ERROR(L"Failed to create event");
        return NULL;
    }
    for (;;)
    {
        LastError = RegNotifyChangeKeyValue(Key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, Event, TRUE);
        if (LastError != ERROR_SUCCESS)
        {
            LOG_ERROR(L"Failed to setup notification", LastError);
            break;
        }
        WCHAR *Value = RegistryQueryString(Key, Name, FALSE);
        if (Value)
        {
            CloseHandle(Event);
            return Value;
        }
        LastError = GetLastError();
        if (LastError != ERROR_FILE_NOT_FOUND && LastError != ERROR_PATH_NOT_FOUND)
            break;
        LONGLONG TimeLeft = Deadline - GetTickCount64();
        if (TimeLeft < 0)
            TimeLeft = 0;
        if (WaitForSingleObject(Event, (DWORD)TimeLeft) != WAIT_OBJECT_0)
        {
            LOG(WINTUN_LOG_ERR, L"Timeout waiting");
            break;
        }
    }
    CloseHandle(Event);
    SetLastError(LastError);
    return NULL;
}

_Return_type_success_(return != FALSE) BOOL
    RegistryQueryDWORD(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _Out_ DWORD *Value, _In_ BOOL Log)
{
    DWORD ValueType, Size = sizeof(DWORD);
    DWORD LastError = RegQueryValueExW(Key, Name, NULL, &ValueType, (BYTE *)Value, &Size);
    if (LastError != ERROR_SUCCESS)
    {
        if (Log)
            LOG_ERROR(L"Querying failed", LastError);
        SetLastError(LastError);
        return FALSE;
    }
    if (ValueType != REG_DWORD)
    {
        LOG(WINTUN_LOG_ERR, L"Value is not a DWORD");
        SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
    }
    if (Size != sizeof(DWORD))
    {
        LOG(WINTUN_LOG_ERR, L"Value size is not 4 bytes");
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    return TRUE;
}

_Return_type_success_(return != FALSE) BOOL
    RegistryQueryDWORDWait(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ DWORD Timeout, _Out_ DWORD *Value)
{
    DWORD LastError;
    ULONGLONG Deadline = GetTickCount64() + Timeout;
    HANDLE Event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!Event)
    {
        LOG_LAST_ERROR(L"Failed to create event");
        return FALSE;
    }
    for (;;)
    {
        LastError = RegNotifyChangeKeyValue(Key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, Event, TRUE);
        if (LastError != ERROR_SUCCESS)
        {
            LOG_ERROR(L"Failed to setup notification", LastError);
            break;
        }
        if (RegistryQueryDWORD(Key, Name, Value, FALSE))
        {
            CloseHandle(Event);
            return TRUE;
        }
        LastError = GetLastError();
        if (LastError != ERROR_FILE_NOT_FOUND && LastError != ERROR_PATH_NOT_FOUND)
            break;
        LONGLONG TimeLeft = Deadline - GetTickCount64();
        if (TimeLeft < 0)
            TimeLeft = 0;
        if (WaitForSingleObject(Event, (DWORD)TimeLeft) != WAIT_OBJECT_0)
        {
            LOG(WINTUN_LOG_ERR, L"Timeout waiting");
            break;
        }
    }
    CloseHandle(Event);
    SetLastError(LastError);
    return FALSE;
}
