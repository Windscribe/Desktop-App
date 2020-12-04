/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include "wintun.h"
#include <IPExport.h>
#include <SetupAPI.h>
#include <Windows.h>

#define MAX_INSTANCE_ID MAX_PATH /* TODO: Is MAX_PATH always enough? */
#define WINTUN_HWID L"Wintun"

void
AdapterInit(void);

/**
 * Wintun adapter descriptor.
 */
typedef struct _WINTUN_ADAPTER
{
    GUID CfgInstanceID;
    WCHAR DevInstanceID[MAX_INSTANCE_ID];
    DWORD LuidIndex;
    DWORD IfType;
    WCHAR Pool[WINTUN_MAX_POOL];
} WINTUN_ADAPTER;

/**
 * @copydoc WINTUN_FREE_ADAPTER_FUNC
 */
void WINAPI
WintunFreeAdapter(_In_ WINTUN_ADAPTER *Adapter);

/**
 * @copydoc WINTUN_CREATE_ADAPTER_FUNC
 */
_Return_type_success_(return != NULL) WINTUN_ADAPTER *WINAPI WintunCreateAdapter(
    _In_z_ const WCHAR *Pool,
    _In_z_ const WCHAR *Name,
    _In_opt_ const GUID *RequestedGUID,
    _Out_opt_ BOOL *RebootRequired);

/**
 * @copydoc WINTUN_DELETE_ADAPTER_FUNC
 */
_Return_type_success_(return != FALSE) BOOL WINAPI WintunDeleteAdapter(
    _In_ const WINTUN_ADAPTER *Adapter,
    _In_ BOOL ForceCloseSessions,
    _Out_opt_ BOOL *RebootRequired);

/**
 * @copydoc WINTUN_DELETE_POOL_DRIVER_FUNC
 */
_Return_type_success_(return != FALSE) BOOL WINAPI
    WintunDeletePoolDriver(_In_z_ const WCHAR *Pool, _Out_opt_ BOOL *RebootRequired);

/**
 * Returns a handle to the adapter device object.
 *
 * @param Adapter       Adapter handle obtained with WintunOpenAdapter or WintunCreateAdapter.
 *
 * @return If the function succeeds, the return value is adapter device object handle. Must be released with
 *         CloseHandle. If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error
 *         information, call GetLastError.
 */
_Return_type_success_(return != INVALID_HANDLE_VALUE) HANDLE WINAPI
    AdapterOpenDeviceObject(_In_ const WINTUN_ADAPTER *Adapter);