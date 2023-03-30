/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

_Check_return_
_Return_type_success_(return != NULL) HANDLE NamespaceTakePoolMutex(_In_z_ const WCHAR *Pool);

_Check_return_
_Return_type_success_(return != NULL) HANDLE NamespaceTakeDriverInstallationMutex(void);

void
NamespaceReleaseMutex(_In_ HANDLE Mutex);

void
NamespaceInit(void);

void
NamespaceDone(void);
