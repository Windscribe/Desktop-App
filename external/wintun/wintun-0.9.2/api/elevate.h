/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

_Return_type_success_(return != FALSE) BOOL ElevateToSystem(void);

_Return_type_success_(return != NULL) HANDLE GetPrimarySystemTokenFromThread(void);
