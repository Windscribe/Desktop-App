/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include "wintun.h"
#include "entry.h"
#include <Windows.h>

extern WINTUN_LOGGER_CALLBACK Logger;

/**
 * @copydoc WINTUN_SET_LOGGER_FUNC
 */
void WINAPI
WintunSetLogger(_In_ WINTUN_LOGGER_CALLBACK NewLogger);

_Post_equals_last_error_ DWORD
LoggerLog(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR *Function, _In_z_ const WCHAR *LogLine);

_Post_equals_last_error_ DWORD
LoggerError(_In_z_ const WCHAR *Function, _In_z_ const WCHAR *Prefix, _In_ DWORD Error);

static inline _Post_equals_last_error_ DWORD
LoggerLastError(_In_z_ const WCHAR *Function, _In_z_ const WCHAR *Prefix)
{
    DWORD LastError = GetLastError();
    LoggerError(Function, Prefix, LastError);
    SetLastError(LastError);
    return LastError;
}

#define __L(x) L##x
#define _L(x) __L(x)
#define LOG(lvl, msg) (LoggerLog((lvl), _L(__FUNCTION__), msg))
#define LOG_ERROR(msg, err) (LoggerError(_L(__FUNCTION__), msg, (err)))
#define LOG_LAST_ERROR(msg) (LoggerLastError(_L(__FUNCTION__), msg))

#define RET_ERROR(Ret, Error) ((Error) == ERROR_SUCCESS ? (Ret) : (SetLastError(Error), 0))

static inline _Return_type_success_(return != NULL) _Ret_maybenull_
    _Post_writable_byte_size_(Size) void *LoggerAlloc(_In_z_ const WCHAR *Function, _In_ DWORD Flags, _In_ SIZE_T Size)
{
    void *Data = HeapAlloc(ModuleHeap, Flags, Size);
    if (!Data)
    {
        LoggerLog(WINTUN_LOG_ERR, Function, L"Out of memory");
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return Data;
}
#define Alloc(Size) LoggerAlloc(_L(__FUNCTION__), 0, Size)
#define Zalloc(Size) LoggerAlloc(_L(__FUNCTION__), HEAP_ZERO_MEMORY, Size)
static inline void
Free(void *Ptr)
{
    if (!Ptr)
        return;
    DWORD LastError = GetLastError();
    HeapFree(ModuleHeap, 0, Ptr);
    SetLastError(LastError);
}