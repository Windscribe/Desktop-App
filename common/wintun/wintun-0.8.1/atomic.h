/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2019 WireGuard LLC. All Rights Reserved.
 */

#pragma once

#include <wdm.h>

static __forceinline VOID
InterlockedSet(_Inout_ _Interlocked_operand_ LONG volatile *Target, _In_ LONG Value)
{
    *Target = Value;
}

static __forceinline VOID
InterlockedSetU(_Inout_ _Interlocked_operand_ ULONG volatile *Target, _In_ ULONG Value)
{
    *Target = Value;
}

static __forceinline VOID
InterlockedSetPointer(_Inout_ _Interlocked_operand_ VOID *volatile *Target, _In_opt_ VOID *Value)
{
    *Target = Value;
}

static __forceinline LONG
InterlockedGet(_In_ _Interlocked_operand_ LONG volatile *Value)
{
    return *Value;
}

static __forceinline ULONG
InterlockedGetU(_In_ _Interlocked_operand_ ULONG volatile *Value)
{
    return *Value;
}

static __forceinline PVOID
InterlockedGetPointer(_In_ _Interlocked_operand_ PVOID volatile *Value)
{
    return *Value;
}

static __forceinline LONG64
InterlockedGet64(_In_ _Interlocked_operand_ LONG64 volatile *Value)
{
#ifdef _WIN64
    return *Value;
#else
    return InterlockedCompareExchange64(Value, 0, 0);
#endif
}