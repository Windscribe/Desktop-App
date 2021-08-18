/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include "wintun.h"
#include <Windows.h>

#define MAX_REG_PATH \
    256 /* Maximum registry path length \
           https://support.microsoft.com/en-us/help/256986/windows-registry-information-for-advanced-users */

/**
 * Opens the specified registry key. It waits for the registry key to become available.
 *
 * @param Key           Handle of the parent registry key. Must be opened with notify access.
 *
 * @param Path          Subpath of the registry key to open. Zero-terminated string of up to MAX_REG_PATH-1 characters.
 *
 * @param Access        A mask that specifies the desired access rights to the key to be opened.
 *
 * @param Timeout       Timeout to wait for the value in milliseconds.
 *
 * @return Key handle on success. If the function fails, the return value is zero. To get extended error information,
 *         call GetLastError.
 */
_Return_type_success_(return != NULL) HKEY
    RegistryOpenKeyWait(_In_ HKEY Key, _In_z_ const WCHAR *Path, _In_ DWORD Access, _In_ DWORD Timeout);

/**
 * Validates and/or sanitizes string value read from registry.
 *
 * @param Buf           On input, it contains a pointer to pointer where the data is stored. The data must be allocated
 *                      using HeapAlloc(ModuleHeap, 0). On output, it contains a pointer to pointer where the sanitized
 *                      data is stored. It must be released with HeapFree(ModuleHeap, 0, *Buf) after use.
 *
 * @param Len           Length of data string in wide characters.
 *
 * @param ValueType     Type of data. Must be either REG_SZ or REG_EXPAND_SZ. REG_MULTI_SZ is treated like REG_SZ; only
 *                      the first string of a multi-string is to be used.
 *
 * @return If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError.
 */
_Return_type_success_(return != FALSE) BOOL
    RegistryGetString(_Inout_ WCHAR **Buf, _In_ DWORD Len, _In_ DWORD ValueType);

/**
 * Validates and/or sanitizes multi-string value read from registry.
 *
 * @param Buf           On input, it contains a pointer to pointer where the data is stored. The data must be allocated
 *                      using HeapAlloc(ModuleHeap, 0). On output, it contains a pointer to pointer where the sanitized
 *                      data is stored. It must be released with HeapFree(ModuleHeap, 0, *Buf) after use.
 *
 * @param Len           Length of data string in wide characters.
 *
 * @param ValueType     Type of data. Must be one of REG_MULTI_SZ, REG_SZ or REG_EXPAND_SZ.
 *
 * @return If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError.
 */
_Return_type_success_(return != FALSE) BOOL
    RegistryGetMultiString(_Inout_ WCHAR **Buf, _In_ DWORD Len, _In_ DWORD ValueType);

/**
 * Reads string value from registry key.
 *
 * @param Key           Handle of the registry key to read from. Must be opened with read access.
 *
 * @param Name          Name of the value to read.
 *
 * @param Value         Pointer to string to retrieve registry value. If the value type is REG_EXPAND_SZ the value is
 *                      expanded using ExpandEnvironmentStrings(). If the value type is REG_MULTI_SZ, only the first
 *                      string from the multi-string is returned. The string must be released with
 *                      HeapFree(ModuleHeap, 0, Value) after use.
 *
 * @Log                 Set to TRUE to log all failures; FALSE to skip logging the innermost errors. Skipping innermost
 *                      errors reduces log clutter when we are using RegistryQueryString() from
 *                      RegistryQueryStringWait() and some errors are expected to occur.
 *
 * @return String with registry value on success; If the function fails, the return value is zero. To get extended error
 *         information, call GetLastError.
 */
_Return_type_success_(
    return != NULL) WCHAR *RegistryQueryString(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ BOOL Log);

/**
 * Reads string value from registry key. It waits for the registry value to become available.
 *
 * @param Key           Handle of the registry key to read from. Must be opened with read and notify access.
 *
 * @param Name          Name of the value to read.
 *
 * @param Timeout       Timeout to wait for the value in milliseconds.
 *
 * @return Registry value. If the value type is REG_EXPAND_SZ the value is expanded using ExpandEnvironmentStrings(). If
 *         the value type is REG_MULTI_SZ, only the first string from the multi-string is returned. The string must be
 *         released with HeapFree(ModuleHeap, 0, Value) after use. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError. Possible errors include the following:
 *         ERROR_INVALID_DATATYPE when the registry value is not a string
 */
_Return_type_success_(
    return != NULL) WCHAR *RegistryQueryStringWait(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ DWORD Timeout);

/**
 * Reads a 32-bit DWORD value from registry key.
 *
 * @param Key           Handle of the registry key to read from. Must be opened with read access.
 *
 * @param Name          Name of the value to read.
 *
 * @param Value         Pointer to DWORD to retrieve registry value.
 *
 * @Log                 Set to TRUE to log all failures; FALSE to skip logging the innermost errors. Skipping innermost
 *                      errors reduces log clutter when we are using RegistryQueryDWORD() from
 *                      RegistryQueryDWORDWait() and some errors are expected to occur.
 *
 * @return If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError.
 */
_Return_type_success_(return != FALSE) BOOL
    RegistryQueryDWORD(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _Out_ DWORD *Value, _In_ BOOL Log);

/**
 * Reads a 32-bit DWORD value from registry key. It waits for the registry value to become available.
 *
 * @param Key           Handle of the registry key to read from. Must be opened with read access.
 *
 * @param Name          Name of the value to read.
 *
 * @param Timeout       Timeout to wait for the value in milliseconds.
 *
 * @param Value         Pointer to DWORD to retrieve registry value.
 *
 * @return If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError. Possible errors include the following:
 *         ERROR_INVALID_DATATYPE when registry value exist but not REG_DWORD type;
 *         ERROR_INVALID_DATA when registry value size is not 4 bytes
 */
_Return_type_success_(return != FALSE) BOOL
    RegistryQueryDWORDWait(_In_ HKEY Key, _In_opt_z_ const WCHAR *Name, _In_ DWORD Timeout, _Out_ DWORD *Value);
