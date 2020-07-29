/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2019 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

typedef enum _LOGGER_LEVEL
{
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERR
} LOGGER_LEVEL;
typedef VOID (*LoggerFunction)(_In_ LOGGER_LEVEL, _In_ const TCHAR *);
VOID
SetLogger(_In_ LoggerFunction NewLogger);

BOOL InstallOrUpdate(VOID);
BOOL Uninstall(VOID);

extern HINSTANCE ResourceModule;
