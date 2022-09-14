/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#include "logger.h"
#include <Windows.h>
#include <wchar.h>

static BOOL CALLBACK
NopLogger(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR *LogLine)
{
    UNREFERENCED_PARAMETER(Level);
    UNREFERENCED_PARAMETER(LogLine);
    return TRUE;
}

WINTUN_LOGGER_CALLBACK Logger = NopLogger;

void CALLBACK
WintunSetLogger(_In_ WINTUN_LOGGER_CALLBACK NewLogger)
{
    if (!NewLogger)
        NewLogger = NopLogger;
    Logger = NewLogger;
}

_Post_equals_last_error_ DWORD
LoggerLog(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR *Function, _In_z_ const WCHAR *LogLine)
{
    DWORD LastError = GetLastError();
    if (Function)
    {
        WCHAR Combined[0x400];
        if (_snwprintf_s(Combined, _countof(Combined), _TRUNCATE, L"%s: %s", Function, LogLine) == -1)
            Logger(Level, LogLine);
        else
            Logger(Level, Combined);
    }
    else
        Logger(Level, LogLine);
    SetLastError(LastError);
    return LastError;
}

_Post_equals_last_error_ DWORD
LoggerError(_In_z_ const WCHAR *Function, _In_z_ const WCHAR *Prefix, _In_ DWORD Error)
{
    WCHAR *SystemMessage = NULL, *FormattedMessage = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        HRESULT_FROM_SETUPAPI(Error),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (void *)&SystemMessage,
        0,
        NULL);
    FormatMessageW(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
        SystemMessage ? L"%4: %1: %3(Code 0x%2!08X!)" : L"%1: Code 0x%2!08X!",
        0,
        0,
        (void *)&FormattedMessage,
        0,
        (va_list *)(DWORD_PTR[]){ (DWORD_PTR)Prefix, (DWORD_PTR)Error, (DWORD_PTR)SystemMessage, (DWORD_PTR)Function });
    if (FormattedMessage)
        Logger(WINTUN_LOG_ERR, FormattedMessage);
    LocalFree(FormattedMessage);
    LocalFree(SystemMessage);
    return Error;
}
