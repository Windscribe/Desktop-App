/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include "wintun.h"
#include <Windows.h>

/**
 * Locates RT_RCDATA resource memory address and size.
 *
 * ResourceName         Name of the RT_RCDATA resource. Use MAKEINTRESOURCEW to locate resource by ID.
 *
 * Size                 Pointer to a variable to receive resource size.
 *
 * @return Resource address on success. If the function fails, the return value is NULL. To get extended error
 *         information, call GetLastError.
 */
_Return_type_success_(return != NULL) _Ret_bytecount_(*Size) const
    void *ResourceGetAddress(_In_z_ const WCHAR *ResourceName, _Out_ DWORD *Size);

/**
 * Copies resource to a file.
 *
 * DestinationPath      File path
 *
 * ResourceName         Name of the RT_RCDATA resource. Use MAKEINTRESOURCEW to locate resource by ID.
 *
 * @return If the function succeeds, the return value is nonzero. If the function fails, the return value is zero. To
 *         get extended error information, call GetLastError.
 */
_Return_type_success_(return != FALSE) BOOL
    ResourceCopyToFile(_In_z_ const WCHAR *DestinationPath, _In_z_ const WCHAR *ResourceName);
