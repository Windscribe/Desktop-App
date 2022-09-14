/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include <Windows.h>

/* TODO: Replace with is_defined. MSVC has issues with the linux kernel varadic macro trick for this. */
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_ARM)
#    define MAYBE_WOW64 1
#else
#    define MAYBE_WOW64 0
#endif
#if defined(_M_AMD64) || defined(_M_ARM64)
#    define ACCEPT_WOW64 1
#else
#    define ACCEPT_WOW64 0
#endif
#ifdef HAVE_WHQL
#    undef HAVE_WHQL
#    define HAVE_WHQL 1
#else
#    define HAVE_WHQL 0
#endif
#pragma warning(disable : 4127) /* conditional expression is constant */

extern HINSTANCE ResourceModule;
extern HANDLE ModuleHeap;
extern SECURITY_ATTRIBUTES SecurityAttributes;
