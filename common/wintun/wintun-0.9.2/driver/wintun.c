/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#include <ntifs.h>
#include <wdm.h>
#include <wdmsec.h>
#include <ndis.h>
#include <ntstrsafe.h>
#include "undocumented.h"

#pragma warning(disable : 4100) /* unreferenced formal parameter */
#pragma warning(disable : 4200) /* nonstandard: zero-sized array in struct/union */
#pragma warning(disable : 4204) /* nonstandard: non-constant aggregate initializer */
#pragma warning(disable : 4221) /* nonstandard: cannot be initialized using address of automatic variable */
#pragma warning(disable : 6320) /* exception-filter expression is the constant EXCEPTION_EXECUTE_HANDLER */

#define NDIS_MINIPORT_VERSION_MIN ((NDIS_MINIPORT_MINIMUM_MAJOR_VERSION << 16) | NDIS_MINIPORT_MINIMUM_MINOR_VERSION)
#define NDIS_MINIPORT_VERSION_MAX ((NDIS_MINIPORT_MAJOR_VERSION << 16) | NDIS_MINIPORT_MINOR_VERSION)

#define TUN_VENDOR_NAME "Wintun Tunnel"
#define TUN_VENDOR_ID 0xFFFFFF00
#define TUN_LINK_SPEED 100000000000ULL /* 100gbps */

/* Memory alignment of packets and rings */
#define TUN_ALIGNMENT sizeof(ULONG)
#define TUN_ALIGN(Size) (((ULONG)(Size) + ((ULONG)TUN_ALIGNMENT - 1)) & ~((ULONG)TUN_ALIGNMENT - 1))
#define TUN_IS_ALIGNED(Size) (!((ULONG)(Size) & ((ULONG)TUN_ALIGNMENT - 1)))
/* Maximum IP packet size */
#define TUN_MAX_IP_PACKET_SIZE 0xFFFF
/* Maximum packet size */
#define TUN_MAX_PACKET_SIZE TUN_ALIGN(sizeof(TUN_PACKET) + TUN_MAX_IP_PACKET_SIZE)
/* Minimum ring capacity. */
#define TUN_MIN_RING_CAPACITY 0x20000 /* 128kiB */
/* Maximum ring capacity. */
#define TUN_MAX_RING_CAPACITY 0x4000000 /* 64MiB */
/* Calculates ring capacity */
#define TUN_RING_CAPACITY(Size) ((Size) - sizeof(TUN_RING) - (TUN_MAX_PACKET_SIZE - TUN_ALIGNMENT))
/* Calculates ring offset modulo capacity */
#define TUN_RING_WRAP(Value, Capacity) ((Value) & (Capacity - 1))

#if REG_DWORD == REG_DWORD_BIG_ENDIAN
#    define HTONS(x) ((USHORT)(x))
#    define HTONL(x) ((ULONG)(x))
#elif REG_DWORD == REG_DWORD_LITTLE_ENDIAN
#    define HTONS(x) ((((USHORT)(x)&0x00ff) << 8) | (((USHORT)(x)&0xff00) >> 8))
#    define HTONL(x) \
        ((((ULONG)(x)&0x000000ff) << 24) | (((ULONG)(x)&0x0000ff00) << 8) | (((ULONG)(x)&0x00ff0000) >> 8) | \
         (((ULONG)(x)&0xff000000) >> 24))
#else
#    error "Unable to determine endianess"
#endif

#define TUN_MEMORY_TAG HTONL('wtun')

typedef struct _TUN_PACKET
{
    /* Size of packet data (TUN_MAX_IP_PACKET_SIZE max) */
    ULONG Size;

    /* Packet data */
    UCHAR _Field_size_bytes_(Size)
    Data[];
} TUN_PACKET;

typedef struct _TUN_RING
{
    /* Byte offset of the first packet in the ring. Its value must be a multiple of TUN_ALIGNMENT and less than ring
     * capacity. */
    volatile ULONG Head;

    /* Byte offset of the first free space in the ring. Its value must be multiple of TUN_ALIGNMENT and less than ring
     * capacity. */
    volatile ULONG Tail;

    /* Non-zero when consumer is in alertable state. */
    volatile LONG Alertable;

    /* Ring data. Its capacity must be a power of 2 + extra TUN_MAX_PACKET_SIZE-TUN_ALIGNMENT space to
     * eliminate need for wrapping. */
    UCHAR Data[];
} TUN_RING;

typedef struct _TUN_REGISTER_RINGS
{
    struct
    {
        /* Size of the ring */
        ULONG RingSize;

        /* Pointer to client allocated ring */
        TUN_RING *Ring;

        /* On send: An event created by the client the Wintun signals after it moves the Tail member of the send ring.
         * On receive: An event created by the client the client will signal when it moves the Tail member of
         * the receive ring if receive ring is alertable. */
        HANDLE TailMoved;
    } Send, Receive;
} TUN_REGISTER_RINGS;

#ifdef _WIN64
typedef struct _TUN_REGISTER_RINGS_32
{
    struct
    {
        /* Size of the ring */
        ULONG RingSize;

        /* 32-bit addres of client allocated ring */
        ULONG Ring;

        /* On send: An event created by the client the Wintun signals after it moves the Tail member of the send ring.
         * On receive: An event created by the client the client will signal when it moves the Tail member of
         * the receive ring if receive ring is alertable. */
        ULONG TailMoved;
    } Send, Receive;
} TUN_REGISTER_RINGS_32;
#endif

/* Register rings hosted by the client.
 * The lpInBuffer and nInBufferSize parameters of DeviceIoControl() must point to an TUN_REGISTER_RINGS struct.
 * Client must wait for this IOCTL to finish before adding packets to the ring. */
#define TUN_IOCTL_REGISTER_RINGS CTL_CODE(51820U, 0x970U, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
/* Force close all open handles to allow for updating. */
#define TUN_IOCTL_FORCE_CLOSE_HANDLES CTL_CODE(51820U, 0x971U, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

typedef struct _TUN_CTX
{
    volatile LONG Running;

    /* Used like RCU. When we're making use of rings, we take a shared lock. When we want to register or release the
     * rings and toggle the state, we take an exclusive lock before toggling the atomic and then releasing. It's similar
     * to setting the atomic and then calling rcu_barrier(). */
    EX_SPIN_LOCK TransitionLock;

    NDIS_HANDLE MiniportAdapterHandle; /* This is actually a pointer to NDIS_MINIPORT_BLOCK struct. */
    DEVICE_OBJECT *FunctionalDeviceObject;
    NDIS_STATISTICS_INFO Statistics;

    struct
    {
        LIST_ENTRY Entry;
        ERESOURCE RegistrationLock;
        FILE_OBJECT *OwningFileObject;
        HANDLE OwningProcessId;
        KEVENT Disconnected;

        struct
        {
            MDL *Mdl;
            TUN_RING *Ring;
            ULONG Capacity;
            KEVENT *TailMoved;
            KSPIN_LOCK Lock;
            ULONG RingTail;
            struct
            {
                NET_BUFFER_LIST *Head, *Tail;
            } ActiveNbls;
        } Send;

        struct
        {
            MDL *Mdl;
            TUN_RING *Ring;
            ULONG Capacity;
            KEVENT *TailMoved;
            HANDLE Thread;
            KSPIN_LOCK Lock;
            struct
            {
                NET_BUFFER_LIST *Head, *Tail;
                KEVENT Empty;
            } ActiveNbls;
        } Receive;
    } Device;

    NDIS_HANDLE NblPool;
} TUN_CTX;

static UINT NdisVersion;
static NDIS_HANDLE NdisMiniportDriverHandle;
static DRIVER_DISPATCH *NdisDispatchDeviceControl, *NdisDispatchClose;
static ERESOURCE TunDispatchCtxGuard, TunDispatchDeviceListLock;
static RTL_STATIC_LIST_HEAD(TunDispatchDeviceList);
static SECURITY_DESCRIPTOR *TunDispatchSecurityDescriptor;

_IRQL_requires_max_(DISPATCH_LEVEL)
static VOID
TunIndicateStatus(_In_ NDIS_HANDLE MiniportAdapterHandle, _In_ NDIS_MEDIA_CONNECT_STATE MediaConnectState)
{
    NDIS_LINK_STATE State = { .Header = { .Type = NDIS_OBJECT_TYPE_DEFAULT,
                                          .Revision = NDIS_LINK_STATE_REVISION_1,
                                          .Size = NDIS_SIZEOF_LINK_STATE_REVISION_1 },
                              .MediaConnectState = MediaConnectState,
                              .MediaDuplexState = MediaDuplexStateFull,
                              .XmitLinkSpeed = TUN_LINK_SPEED,
                              .RcvLinkSpeed = TUN_LINK_SPEED,
                              .PauseFunctions = NdisPauseFunctionsUnsupported };

    NDIS_STATUS_INDICATION Indication = { .Header = { .Type = NDIS_OBJECT_TYPE_STATUS_INDICATION,
                                                      .Revision = NDIS_STATUS_INDICATION_REVISION_1,
                                                      .Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1 },
                                          .SourceHandle = MiniportAdapterHandle,
                                          .StatusCode = NDIS_STATUS_LINK_STATE,
                                          .StatusBuffer = &State,
                                          .StatusBufferSize = sizeof(State) };

    NdisMIndicateStatusEx(MiniportAdapterHandle, &Indication);
}

/* Send: We should not modify NET_BUFFER_LIST_NEXT_NBL(Nbl) to prevent fragmented NBLs to separate.
 * Receive: NDIS may change NET_BUFFER_LIST_NEXT_NBL(Nbl) at will between the NdisMIndicateReceiveNetBufferLists() and
 * MINIPORT_RETURN_NET_BUFFER_LISTS calls. Therefore, we use our own ->Next pointer for book-keeping. */
#define NET_BUFFER_LIST_NEXT_NBL_EX(Nbl) (NET_BUFFER_LIST_MINIPORT_RESERVED(Nbl)[1])

static VOID
TunNblSetOffsetAndMarkActive(_Inout_ NET_BUFFER_LIST *Nbl, _In_ ULONG Offset)
{
    ASSERT(TUN_IS_ALIGNED(Offset)); /* Alignment ensures bit 0 will be 0 (0=active, 1=completed). */
    NET_BUFFER_LIST_MINIPORT_RESERVED(Nbl)[0] = (VOID *)Offset;
}

static ULONG
TunNblGetOffset(_In_ NET_BUFFER_LIST *Nbl)
{
    return (ULONG)((ULONG_PTR)(NET_BUFFER_LIST_MINIPORT_RESERVED(Nbl)[0]) & ~((ULONG_PTR)TUN_ALIGNMENT - 1));
}

static VOID
TunNblMarkCompleted(_Inout_ NET_BUFFER_LIST *Nbl)
{
    *(ULONG_PTR *)&NET_BUFFER_LIST_MINIPORT_RESERVED(Nbl)[0] |= 1;
}

static BOOLEAN
TunNblIsCompleted(_In_ NET_BUFFER_LIST *Nbl)
{
    return (ULONG_PTR)(NET_BUFFER_LIST_MINIPORT_RESERVED(Nbl)[0]) & 1;
}

static MINIPORT_SEND_NET_BUFFER_LISTS TunSendNetBufferLists;
_Use_decl_annotations_
static VOID
TunSendNetBufferLists(
    NDIS_HANDLE MiniportAdapterContext,
    NET_BUFFER_LIST *NetBufferLists,
    NDIS_PORT_NUMBER PortNumber,
    ULONG SendFlags)
{
    TUN_CTX *Ctx = (TUN_CTX *)MiniportAdapterContext;
    LONG64 SentPacketsCount = 0, SentPacketsSize = 0, ErrorPacketsCount = 0, DiscardedPacketsCount = 0;

    /* Measure NBLs. */
    ULONG PacketsCount = 0, RequiredRingSpace = 0;
    for (NET_BUFFER_LIST *Nbl = NetBufferLists; Nbl; Nbl = NET_BUFFER_LIST_NEXT_NBL(Nbl))
    {
        for (NET_BUFFER *Nb = NET_BUFFER_LIST_FIRST_NB(Nbl); Nb; Nb = NET_BUFFER_NEXT_NB(Nb))
        {
            PacketsCount++;
            UINT PacketSize = NET_BUFFER_DATA_LENGTH(Nb);
            if (PacketSize > TUN_MAX_IP_PACKET_SIZE)
                continue; /* The same condition holds down below, where we `goto skipPacket`. */
            RequiredRingSpace += TUN_ALIGN(sizeof(TUN_PACKET) + PacketSize);
        }
    }

    KIRQL Irql = ExAcquireSpinLockShared(&Ctx->TransitionLock);
    NDIS_STATUS Status;
    if ((Status = NDIS_STATUS_PAUSED, !ReadAcquire(&Ctx->Running)) ||
        (Status = NDIS_STATUS_MEDIA_DISCONNECTED, KeReadStateEvent(&Ctx->Device.Disconnected)))
        goto skipNbl;

    TUN_RING *Ring = Ctx->Device.Send.Ring;
    ULONG RingCapacity = Ctx->Device.Send.Capacity;

    /* Allocate space for packets in the ring. */
    ULONG RingHead = ReadULongAcquire(&Ring->Head);
    if (Status = NDIS_STATUS_ADAPTER_NOT_READY, RingHead >= RingCapacity)
        goto skipNbl;

    KLOCK_QUEUE_HANDLE LockHandle;
    KeAcquireInStackQueuedSpinLock(&Ctx->Device.Send.Lock, &LockHandle);

    ULONG RingTail = Ctx->Device.Send.RingTail;
    ASSERT(RingTail < RingCapacity);

    ULONG RingSpace = TUN_RING_WRAP(RingHead - RingTail - TUN_ALIGNMENT, RingCapacity);
    if (Status = NDIS_STATUS_BUFFER_OVERFLOW, RingSpace < RequiredRingSpace)
        goto cleanupKeReleaseInStackQueuedSpinLock;

    Ctx->Device.Send.RingTail = TUN_RING_WRAP(RingTail + RequiredRingSpace, RingCapacity);
    TunNblSetOffsetAndMarkActive(NetBufferLists, Ctx->Device.Send.RingTail);
    *(Ctx->Device.Send.ActiveNbls.Head ? &NET_BUFFER_LIST_NEXT_NBL_EX(Ctx->Device.Send.ActiveNbls.Tail)
                                       : &Ctx->Device.Send.ActiveNbls.Head) = NetBufferLists;
    Ctx->Device.Send.ActiveNbls.Tail = NetBufferLists;

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    /* Copy packets. */
    for (NET_BUFFER_LIST *Nbl = NetBufferLists; Nbl; Nbl = NET_BUFFER_LIST_NEXT_NBL(Nbl))
    {
        for (NET_BUFFER *Nb = NET_BUFFER_LIST_FIRST_NB(Nbl); Nb; Nb = NET_BUFFER_NEXT_NB(Nb))
        {
            UINT PacketSize = NET_BUFFER_DATA_LENGTH(Nb);
            if (Status = NDIS_STATUS_INVALID_LENGTH, PacketSize > TUN_MAX_IP_PACKET_SIZE)
                goto skipPacket;

            TUN_PACKET *Packet = (TUN_PACKET *)(Ring->Data + RingTail);
            Packet->Size = PacketSize;
            void *NbData = NdisGetDataBuffer(Nb, PacketSize, Packet->Data, 1, 0);
            if (!NbData)
            {
                /* The space for the packet has already been allocated in the ring. Write a zero-packet rather than
                 * fixing the gap in the ring. */
                NdisZeroMemory(Packet->Data, PacketSize);
                DiscardedPacketsCount++;
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_FAILURE;
            }
            else
            {
                if (NbData != Packet->Data)
                    NdisMoveMemory(Packet->Data, NbData, PacketSize);
                SentPacketsCount++;
                SentPacketsSize += PacketSize;
            }

            RingTail = TUN_RING_WRAP(RingTail + TUN_ALIGN(sizeof(TUN_PACKET) + PacketSize), RingCapacity);
            continue;

        skipPacket:
            ErrorPacketsCount++;
            NET_BUFFER_LIST_STATUS(Nbl) = Status;
        }
    }
    ASSERT(RingTail == TunNblGetOffset(NetBufferLists));
    TunNblMarkCompleted(NetBufferLists);

    /* Adjust the ring tail. */
    KeAcquireInStackQueuedSpinLock(&Ctx->Device.Send.Lock, &LockHandle);
    while (Ctx->Device.Send.ActiveNbls.Head && TunNblIsCompleted(Ctx->Device.Send.ActiveNbls.Head))
    {
        NET_BUFFER_LIST *CompletedNbl = Ctx->Device.Send.ActiveNbls.Head;
        Ctx->Device.Send.ActiveNbls.Head = NET_BUFFER_LIST_NEXT_NBL_EX(CompletedNbl);
        WriteULongRelease(&Ring->Tail, TunNblGetOffset(CompletedNbl));
        KeSetEvent(Ctx->Device.Send.TailMoved, IO_NETWORK_INCREMENT, FALSE);
        NdisMSendNetBufferListsComplete(
            Ctx->MiniportAdapterHandle, CompletedNbl, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
    }
    KeReleaseInStackQueuedSpinLock(&LockHandle);
    ExReleaseSpinLockShared(&Ctx->TransitionLock, Irql);
    goto updateStatistics;

cleanupKeReleaseInStackQueuedSpinLock:
    KeReleaseInStackQueuedSpinLock(&LockHandle);
skipNbl:
    for (NET_BUFFER_LIST *Nbl = NetBufferLists; Nbl; Nbl = NET_BUFFER_LIST_NEXT_NBL(Nbl))
        NET_BUFFER_LIST_STATUS(Nbl) = Status;
    DiscardedPacketsCount += PacketsCount;
    ExReleaseSpinLockShared(&Ctx->TransitionLock, Irql);
    NdisMSendNetBufferListsComplete(Ctx->MiniportAdapterHandle, NetBufferLists, 0);
updateStatistics:
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutOctets, SentPacketsSize);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutUcastOctets, SentPacketsSize);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutUcastPkts, SentPacketsCount);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifOutErrors, ErrorPacketsCount);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifOutDiscards, DiscardedPacketsCount);
}

static MINIPORT_CANCEL_SEND TunCancelSend;
_Use_decl_annotations_
static VOID
TunCancelSend(NDIS_HANDLE MiniportAdapterContext, PVOID CancelId)
{
}

static MINIPORT_RETURN_NET_BUFFER_LISTS TunReturnNetBufferLists;
_Use_decl_annotations_
static VOID
TunReturnNetBufferLists(NDIS_HANDLE MiniportAdapterContext, PNET_BUFFER_LIST NetBufferLists, ULONG ReturnFlags)
{
    TUN_CTX *Ctx = (TUN_CTX *)MiniportAdapterContext;
    TUN_RING *Ring = Ctx->Device.Receive.Ring;

    LONG64 ReceivedPacketsCount = 0, ReceivedPacketsSize = 0, ErrorPacketsCount = 0;
    for (NET_BUFFER_LIST *Nbl = NetBufferLists, *NextNbl; Nbl; Nbl = NextNbl)
    {
        NextNbl = NET_BUFFER_LIST_NEXT_NBL(Nbl);

        if (NT_SUCCESS(NET_BUFFER_LIST_STATUS(Nbl)))
        {
            ReceivedPacketsCount++;
            ReceivedPacketsSize += NET_BUFFER_LIST_FIRST_NB(Nbl)->DataLength;
        }
        else
            ErrorPacketsCount++;

        TunNblMarkCompleted(Nbl);
        for (;;)
        {
            KLOCK_QUEUE_HANDLE LockHandle;
            KeAcquireInStackQueuedSpinLock(&Ctx->Device.Receive.Lock, &LockHandle);
            NET_BUFFER_LIST *CompletedNbl = Ctx->Device.Receive.ActiveNbls.Head;
            if (!CompletedNbl || !TunNblIsCompleted(CompletedNbl))
            {
                KeReleaseInStackQueuedSpinLock(&LockHandle);
                break;
            }
            Ctx->Device.Receive.ActiveNbls.Head = NET_BUFFER_LIST_NEXT_NBL_EX(CompletedNbl);
            if (!Ctx->Device.Receive.ActiveNbls.Head)
                KeSetEvent(&Ctx->Device.Receive.ActiveNbls.Empty, IO_NO_INCREMENT, FALSE);
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            WriteULongRelease(&Ring->Head, TunNblGetOffset(CompletedNbl));
            NdisFreeNetBufferList(CompletedNbl);
        }
    }

    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCInOctets, ReceivedPacketsSize);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCInUcastOctets, ReceivedPacketsSize);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifHCInUcastPkts, ReceivedPacketsCount);
    InterlockedAddNoFence64((LONG64 *)&Ctx->Statistics.ifInErrors, ErrorPacketsCount);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Function_class_(KSTART_ROUTINE)
static VOID
TunProcessReceiveData(_Inout_ TUN_CTX *Ctx)
{
    KeSetPriorityThread(KeGetCurrentThread(), 1);

    TUN_RING *Ring = Ctx->Device.Receive.Ring;
    ULONG RingCapacity = Ctx->Device.Receive.Capacity;
    LARGE_INTEGER Frequency;
    KeQueryPerformanceCounter(&Frequency);
    ULONG64 SpinMax = Frequency.QuadPart / 1000 / 10; /* 1/10 ms */
    VOID *Events[] = { &Ctx->Device.Disconnected, Ctx->Device.Receive.TailMoved };
    ASSERT(RTL_NUMBER_OF(Events) <= THREAD_WAIT_OBJECTS);

    ULONG RingHead = ReadULongAcquire(&Ring->Head);
    if (RingHead >= RingCapacity)
        goto cleanup;

    while (!KeReadStateEvent(&Ctx->Device.Disconnected))
    {
        /* Get next packet from the ring. */
        ULONG RingTail = ReadULongAcquire(&Ring->Tail);
        if (RingHead == RingTail)
        {
            LARGE_INTEGER SpinStart = KeQueryPerformanceCounter(NULL);
            for (;;)
            {
                RingTail = ReadULongAcquire(&Ring->Tail);
                if (RingTail != RingHead)
                    break;
                if (KeReadStateEvent(&Ctx->Device.Disconnected))
                    break;
                LARGE_INTEGER SpinNow = KeQueryPerformanceCounter(NULL);
                if ((ULONG64)SpinNow.QuadPart - (ULONG64)SpinStart.QuadPart >= SpinMax)
                    break;
                ZwYieldExecution();
            }
            if (RingHead == RingTail)
            {
                WriteRelease(&Ring->Alertable, TRUE);
                RingTail = ReadULongAcquire(&Ring->Tail);
                if (RingHead == RingTail)
                {
                    KeWaitForMultipleObjects(
                        RTL_NUMBER_OF(Events), Events, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
                    WriteRelease(&Ring->Alertable, FALSE);
                    continue;
                }
                WriteRelease(&Ring->Alertable, FALSE);
                KeClearEvent(Ctx->Device.Receive.TailMoved);
            }
        }
        if (RingTail >= RingCapacity)
            break;

        ULONG RingContent = TUN_RING_WRAP(RingTail - RingHead, RingCapacity);
        if (RingContent < sizeof(TUN_PACKET))
            break;

        TUN_PACKET *Packet = (TUN_PACKET *)(Ring->Data + RingHead);
        ULONG PacketSize = *(volatile ULONG *)&Packet->Size;
        if (PacketSize > TUN_MAX_IP_PACKET_SIZE)
            break;

        ULONG AlignedPacketSize = TUN_ALIGN(sizeof(TUN_PACKET) + PacketSize);
        if (AlignedPacketSize > RingContent)
            break;

        ULONG NblFlags;
        USHORT NblProto;
        if (PacketSize >= 20 && Packet->Data[0] >> 4 == 4)
        {
            NblFlags = NDIS_NBL_FLAGS_IS_IPV4;
            NblProto = HTONS(NDIS_ETH_TYPE_IPV4);
        }
        else if (PacketSize >= 40 && Packet->Data[0] >> 4 == 6)
        {
            NblFlags = NDIS_NBL_FLAGS_IS_IPV6;
            NblProto = HTONS(NDIS_ETH_TYPE_IPV6);
        }
        else
            break;

        RingHead = TUN_RING_WRAP(RingHead + AlignedPacketSize, RingCapacity);

        NET_BUFFER_LIST *Nbl = NdisAllocateNetBufferAndNetBufferList(
            Ctx->NblPool, 0, 0, Ctx->Device.Receive.Mdl, (ULONG)(Packet->Data - (UCHAR *)Ring), PacketSize);
        if (!Nbl)
            goto skipNbl;
        Nbl->SourceHandle = Ctx->MiniportAdapterHandle;
        NdisSetNblFlag(Nbl, NblFlags);
        NET_BUFFER_LIST_INFO(Nbl, NetBufferListFrameType) = (PVOID)NblProto;
        NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_SUCCESS;
        TunNblSetOffsetAndMarkActive(Nbl, RingHead);

        KIRQL Irql = ExAcquireSpinLockShared(&Ctx->TransitionLock);
        if (!ReadAcquire(&Ctx->Running))
            goto cleanupNbl;

        KLOCK_QUEUE_HANDLE LockHandle;
        KeAcquireInStackQueuedSpinLock(&Ctx->Device.Receive.Lock, &LockHandle);
        if (Ctx->Device.Receive.ActiveNbls.Head)
            NET_BUFFER_LIST_NEXT_NBL_EX(Ctx->Device.Receive.ActiveNbls.Tail) = Nbl;
        else
        {
            KeClearEvent(&Ctx->Device.Receive.ActiveNbls.Empty);
            Ctx->Device.Receive.ActiveNbls.Head = Nbl;
        }
        Ctx->Device.Receive.ActiveNbls.Tail = Nbl;
        KeReleaseInStackQueuedSpinLock(&LockHandle);

        NdisMIndicateReceiveNetBufferLists(
            Ctx->MiniportAdapterHandle,
            Nbl,
            NDIS_DEFAULT_PORT_NUMBER,
            1,
            NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL | NDIS_RECEIVE_FLAGS_SINGLE_ETHER_TYPE);

        ExReleaseSpinLockShared(&Ctx->TransitionLock, Irql);
        continue;

    cleanupNbl:
        ExReleaseSpinLockShared(&Ctx->TransitionLock, Irql);
        NdisFreeNetBufferList(Nbl);
    skipNbl:
        InterlockedIncrementNoFence64((LONG64 *)&Ctx->Statistics.ifInDiscards);
        KeWaitForSingleObject(&Ctx->Device.Receive.ActiveNbls.Empty, Executive, KernelMode, FALSE, NULL);
        WriteULongRelease(&Ring->Head, RingHead);
    }

    /* Wait for all NBLs to return: 1. To prevent race between proceeding and invalidating ring head. 2. To have
     * TunDispatchUnregisterBuffers() implicitly wait before releasing ring MDL used by NBL(s). */
    KeWaitForSingleObject(&Ctx->Device.Receive.ActiveNbls.Empty, Executive, KernelMode, FALSE, NULL);
cleanup:
    WriteULongRelease(&Ring->Head, MAXULONG);
}

#define IS_POW2(x) ((x) && !((x) & ((x)-1)))

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
static NTSTATUS
TunRegisterBuffers(_Inout_ TUN_CTX *Ctx, _Inout_ IRP *Irp)
{
    NTSTATUS Status = STATUS_ALREADY_INITIALIZED;
    IO_STACK_LOCATION *Stack = IoGetCurrentIrpStackLocation(Irp);

    if (!ExAcquireResourceExclusiveLite(&Ctx->Device.RegistrationLock, FALSE))
        return Status;

    if (Ctx->Device.OwningFileObject)
        goto cleanupMutex;
    Ctx->Device.OwningFileObject = Stack->FileObject;

    TUN_REGISTER_RINGS Rrb;
    if (Stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(Rrb))
        NdisMoveMemory(&Rrb, Irp->AssociatedIrp.SystemBuffer, sizeof(Rrb));
#ifdef _WIN64
    else if (
        IoIs32bitProcess(Irp) && Stack->Parameters.DeviceIoControl.InputBufferLength == sizeof(TUN_REGISTER_RINGS_32))
    {
        TUN_REGISTER_RINGS_32 *Rrb32 = Irp->AssociatedIrp.SystemBuffer;
        Rrb.Send.RingSize = Rrb32->Send.RingSize;
        Rrb.Send.Ring = (TUN_RING *)Rrb32->Send.Ring;
        Rrb.Send.TailMoved = (HANDLE)Rrb32->Send.TailMoved;
        Rrb.Receive.RingSize = Rrb32->Receive.RingSize;
        Rrb.Receive.Ring = (TUN_RING *)Rrb32->Receive.Ring;
        Rrb.Receive.TailMoved = (HANDLE)Rrb32->Receive.TailMoved;
    }
#endif
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanupResetOwner;
    }

    Ctx->Device.Send.Capacity = TUN_RING_CAPACITY(Rrb.Send.RingSize);
    if (Status = STATUS_INVALID_PARAMETER,
        (Ctx->Device.Send.Capacity < TUN_MIN_RING_CAPACITY || Ctx->Device.Send.Capacity > TUN_MAX_RING_CAPACITY ||
         !IS_POW2(Ctx->Device.Send.Capacity) || !Rrb.Send.TailMoved || !Rrb.Send.Ring))
        goto cleanupResetOwner;

    if (!NT_SUCCESS(
            Status = ObReferenceObjectByHandle(
                Rrb.Send.TailMoved,
                /* We will not wait on send ring tail moved event. */
                EVENT_MODIFY_STATE,
                *ExEventObjectType,
                UserMode,
                &Ctx->Device.Send.TailMoved,
                NULL)))
        goto cleanupResetOwner;

    Ctx->Device.Send.Mdl = IoAllocateMdl(Rrb.Send.Ring, Rrb.Send.RingSize, FALSE, FALSE, NULL);
    if (Status = STATUS_INSUFFICIENT_RESOURCES, !Ctx->Device.Send.Mdl)
        goto cleanupSendTailMoved;
    try
    {
        Status = STATUS_INVALID_USER_BUFFER;
        MmProbeAndLockPages(Ctx->Device.Send.Mdl, UserMode, IoWriteAccess);
    }
    except(EXCEPTION_EXECUTE_HANDLER) { goto cleanupSendMdl; }

    Ctx->Device.Send.Ring =
        MmGetSystemAddressForMdlSafe(Ctx->Device.Send.Mdl, NormalPagePriority | MdlMappingNoExecute);
    if (Status = STATUS_INSUFFICIENT_RESOURCES, !Ctx->Device.Send.Ring)
        goto cleanupSendUnlockPages;

    Ctx->Device.Send.RingTail = ReadULongAcquire(&Ctx->Device.Send.Ring->Tail);
    if (Status = STATUS_INVALID_PARAMETER, Ctx->Device.Send.RingTail >= Ctx->Device.Send.Capacity)
        goto cleanupSendUnlockPages;

    Ctx->Device.Receive.Capacity = TUN_RING_CAPACITY(Rrb.Receive.RingSize);
    if (Status = STATUS_INVALID_PARAMETER,
        (Ctx->Device.Receive.Capacity < TUN_MIN_RING_CAPACITY || Ctx->Device.Receive.Capacity > TUN_MAX_RING_CAPACITY ||
         !IS_POW2(Ctx->Device.Receive.Capacity) || !Rrb.Receive.TailMoved || !Rrb.Receive.Ring))
        goto cleanupSendUnlockPages;

    if (!NT_SUCCESS(
            Status = ObReferenceObjectByHandle(
                Rrb.Receive.TailMoved,
                /* We need to clear receive ring TailMoved event on transition to non-alertable state. */
                SYNCHRONIZE | EVENT_MODIFY_STATE,
                *ExEventObjectType,
                UserMode,
                &Ctx->Device.Receive.TailMoved,
                NULL)))
        goto cleanupSendUnlockPages;

    Ctx->Device.Receive.Mdl = IoAllocateMdl(Rrb.Receive.Ring, Rrb.Receive.RingSize, FALSE, FALSE, NULL);
    if (Status = STATUS_INSUFFICIENT_RESOURCES, !Ctx->Device.Receive.Mdl)
        goto cleanupReceiveTailMoved;
    try
    {
        Status = STATUS_INVALID_USER_BUFFER;
        MmProbeAndLockPages(Ctx->Device.Receive.Mdl, UserMode, IoWriteAccess);
    }
    except(EXCEPTION_EXECUTE_HANDLER) { goto cleanupReceiveMdl; }

    Ctx->Device.Receive.Ring =
        MmGetSystemAddressForMdlSafe(Ctx->Device.Receive.Mdl, NormalPagePriority | MdlMappingNoExecute);
    if (Status = STATUS_INSUFFICIENT_RESOURCES, !Ctx->Device.Receive.Ring)
        goto cleanupReceiveUnlockPages;

    KeClearEvent(&Ctx->Device.Disconnected);

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    if (Status = NDIS_STATUS_FAILURE,
        !NT_SUCCESS(PsCreateSystemThread(
            &Ctx->Device.Receive.Thread, THREAD_ALL_ACCESS, &ObjectAttributes, NULL, NULL, TunProcessReceiveData, Ctx)))
        goto cleanupFlagsConnected;

    Ctx->Device.OwningProcessId = PsGetCurrentProcessId();
    InitializeListHead(&Ctx->Device.Entry);
    ExAcquireResourceExclusiveLite(&TunDispatchDeviceListLock, TRUE);
    InsertTailList(&TunDispatchDeviceList, &Ctx->Device.Entry);
    ExReleaseResourceLite(&TunDispatchDeviceListLock);

    ExReleaseResourceLite(&Ctx->Device.RegistrationLock);
    TunIndicateStatus(Ctx->MiniportAdapterHandle, MediaConnectStateConnected);
    return STATUS_SUCCESS;

cleanupFlagsConnected:
    KeSetEvent(&Ctx->Device.Disconnected, IO_NO_INCREMENT, FALSE);
    ExReleaseSpinLockExclusive(
        &Ctx->TransitionLock,
        ExAcquireSpinLockExclusive(&Ctx->TransitionLock)); /* Ensure above change is visible to all readers. */
cleanupReceiveUnlockPages:
    MmUnlockPages(Ctx->Device.Receive.Mdl);
cleanupReceiveMdl:
    IoFreeMdl(Ctx->Device.Receive.Mdl);
cleanupReceiveTailMoved:
    ObDereferenceObject(Ctx->Device.Receive.TailMoved);
cleanupSendUnlockPages:
    MmUnlockPages(Ctx->Device.Send.Mdl);
cleanupSendMdl:
    IoFreeMdl(Ctx->Device.Send.Mdl);
cleanupSendTailMoved:
    ObDereferenceObject(Ctx->Device.Send.TailMoved);
cleanupResetOwner:
    Ctx->Device.OwningFileObject = NULL;
cleanupMutex:
    ExReleaseResourceLite(&Ctx->Device.RegistrationLock);
    return Status;
}

#define TUN_FORCE_UNREGISTRATION ((FILE_OBJECT *)-1)
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID
TunUnregisterBuffers(_Inout_ TUN_CTX *Ctx, _In_ FILE_OBJECT *Owner)
{
    if (!Owner)
        return;
    ExAcquireResourceExclusiveLite(&Ctx->Device.RegistrationLock, TRUE);
    if (!Ctx->Device.OwningFileObject || (Owner != TUN_FORCE_UNREGISTRATION && Ctx->Device.OwningFileObject != Owner))
    {
        ExReleaseResourceLite(&Ctx->Device.RegistrationLock);
        return;
    }
    Ctx->Device.OwningFileObject = NULL;

    ExAcquireResourceExclusiveLite(&TunDispatchDeviceListLock, TRUE);
    RemoveEntryList(&Ctx->Device.Entry);
    ExReleaseResourceLite(&TunDispatchDeviceListLock);

    TunIndicateStatus(Ctx->MiniportAdapterHandle, MediaConnectStateDisconnected);

    KeSetEvent(&Ctx->Device.Disconnected, IO_NO_INCREMENT, FALSE);
    ExReleaseSpinLockExclusive(
        &Ctx->TransitionLock,
        ExAcquireSpinLockExclusive(&Ctx->TransitionLock)); /* Ensure above change is visible to all readers. */

    PKTHREAD ThreadObject;
    if (NT_SUCCESS(
            ObReferenceObjectByHandle(Ctx->Device.Receive.Thread, SYNCHRONIZE, NULL, KernelMode, &ThreadObject, NULL)))
    {
        KeWaitForSingleObject(ThreadObject, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(ThreadObject);
    }
    ZwClose(Ctx->Device.Receive.Thread);

    WriteULongRelease(&Ctx->Device.Send.Ring->Tail, MAXULONG);
    KeSetEvent(Ctx->Device.Send.TailMoved, IO_NO_INCREMENT, FALSE);

    MmUnlockPages(Ctx->Device.Receive.Mdl);
    IoFreeMdl(Ctx->Device.Receive.Mdl);
    ObDereferenceObject(Ctx->Device.Receive.TailMoved);
    MmUnlockPages(Ctx->Device.Send.Mdl);
    IoFreeMdl(Ctx->Device.Send.Mdl);
    ObDereferenceObject(Ctx->Device.Send.TailMoved);

    ExReleaseResourceLite(&Ctx->Device.RegistrationLock);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static BOOLEAN
TunForceHandlesClosed(_Inout_ DEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    PEPROCESS Process;
    KAPC_STATE ApcState;
    PVOID Object = NULL;
    ULONG VerifierFlags = 0;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    SYSTEM_HANDLE_INFORMATION_EX *HandleTable = NULL;
    BOOLEAN DidClose = FALSE;

    MmIsVerifierEnabled(&VerifierFlags);

    for (ULONG Size = 0, RequestedSize;
         (Status = ZwQuerySystemInformation(SystemExtendedHandleInformation, HandleTable, Size, &RequestedSize)) ==
         STATUS_INFO_LENGTH_MISMATCH;
         Size = RequestedSize)
    {
        if (HandleTable)
            ExFreePoolWithTag(HandleTable, TUN_MEMORY_TAG);
        HandleTable = ExAllocatePoolWithTag(PagedPool, RequestedSize, TUN_MEMORY_TAG);
        if (!HandleTable)
            return FALSE;
    }
    if (!NT_SUCCESS(Status) || !HandleTable)
        goto cleanup;

    HANDLE CurrentProcessId = PsGetCurrentProcessId();
    for (ULONG_PTR Index = 0; Index < HandleTable->NumberOfHandles; ++Index)
    {
        FILE_OBJECT *FileObject = HandleTable->Handles[Index].Object;
        if (!FileObject || FileObject->Type != 5 || FileObject->DeviceObject != DeviceObject)
            continue;
        HANDLE ProcessId = HandleTable->Handles[Index].UniqueProcessId;
        if (ProcessId == CurrentProcessId)
            continue;
        Status = PsLookupProcessByProcessId(ProcessId, &Process);
        if (!NT_SUCCESS(Status))
            continue;
        KeStackAttachProcess(Process, &ApcState);
        if (!VerifierFlags)
            Status = ObReferenceObjectByHandle(
                HandleTable->Handles[Index].HandleValue, 0, NULL, UserMode, &Object, &HandleInfo);
        if (NT_SUCCESS(Status))
        {
            if (VerifierFlags || Object == FileObject)
                ObCloseHandle(HandleTable->Handles[Index].HandleValue, UserMode);
            if (!VerifierFlags)
                ObfDereferenceObject(Object);
            DidClose = TRUE;
        }
        KeUnstackDetachProcess(&ApcState);
        ObfDereferenceObject(Process);
    }
cleanup:
    if (HandleTable)
        ExFreePoolWithTag(HandleTable, TUN_MEMORY_TAG);
    return DidClose;
}

static NTSTATUS TunInitializeDispatchSecurityDescriptor(VOID)
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID LocalSystem = { 0 };
    if (!NT_SUCCESS(Status = RtlInitializeSid(&LocalSystem, &NtAuthority, 1)))
        return Status;
    *RtlSubAuthoritySid(&LocalSystem, 0) = SECURITY_LOCAL_SYSTEM_RID;
    struct
    {
        ACL Dacl;
        ACCESS_ALLOWED_ACE AceFiller;
        SID SidFiller;
    } DaclStorage = { 0 };
    if (!NT_SUCCESS(Status = RtlCreateAcl(&DaclStorage.Dacl, sizeof(DaclStorage), ACL_REVISION)))
        return Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    RtlMapGenericMask(&AccessMask, IoGetFileObjectGenericMapping());
    if (!NT_SUCCESS(Status = RtlAddAccessAllowedAce(&DaclStorage.Dacl, ACL_REVISION, AccessMask, &LocalSystem)))
        return Status;
    SECURITY_DESCRIPTOR SecurityDescriptor = { 0 };
    if (!NT_SUCCESS(Status = RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION)))
        return Status;
    if (!NT_SUCCESS(Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, &DaclStorage.Dacl, FALSE)))
        return Status;
    SecurityDescriptor.Control |= SE_DACL_PROTECTED;
    ULONG RequiredBytes = 0;
    Status = RtlAbsoluteToSelfRelativeSD(&SecurityDescriptor, NULL, &RequiredBytes);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return NT_SUCCESS(Status) ? STATUS_INSUFFICIENT_RESOURCES : Status;
    TunDispatchSecurityDescriptor = ExAllocatePoolWithTag(NonPagedPoolNx, RequiredBytes, TUN_MEMORY_TAG);
    if (!TunDispatchSecurityDescriptor)
        return STATUS_INSUFFICIENT_RESOURCES;
    Status = RtlAbsoluteToSelfRelativeSD(&SecurityDescriptor, TunDispatchSecurityDescriptor, &RequiredBytes);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(TunDispatchSecurityDescriptor, TUN_MEMORY_TAG);
        return Status;
    }
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID
TunProcessNotification(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
    if (Create)
        return;
    ExAcquireSharedStarveExclusive(&TunDispatchDeviceListLock, TRUE);
    TUN_CTX *Ctx = NULL;
    for (LIST_ENTRY *Entry = TunDispatchDeviceList.Flink; Entry != &TunDispatchDeviceList; Entry = Entry->Flink)
    {
        TUN_CTX *Candidate = CONTAINING_RECORD(Entry, TUN_CTX, Device.Entry);
        if (Candidate->Device.OwningProcessId == ProcessId)
        {
            Ctx = Candidate;
            break;
        }
    }
    ExReleaseResourceLite(&TunDispatchDeviceListLock);
    if (!Ctx)
        return;

    TunUnregisterBuffers(Ctx, TUN_FORCE_UNREGISTRATION);
}

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
static DRIVER_DISPATCH_PAGED TunDispatchDeviceControl;
_Use_decl_annotations_
static NTSTATUS
TunDispatchDeviceControl(DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
    IO_STACK_LOCATION *Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->Parameters.DeviceIoControl.IoControlCode != TUN_IOCTL_REGISTER_RINGS &&
        Stack->Parameters.DeviceIoControl.IoControlCode != TUN_IOCTL_FORCE_CLOSE_HANDLES)
        return NdisDispatchDeviceControl(DeviceObject, Irp);

    SECURITY_SUBJECT_CONTEXT SubjectContext;
    SeCaptureSubjectContext(&SubjectContext);
    NTSTATUS Status;
    ACCESS_MASK GrantedAccess;
    BOOLEAN HasAccess = SeAccessCheck(
        TunDispatchSecurityDescriptor,
        &SubjectContext,
        FALSE,
        FILE_WRITE_DATA,
        0,
        NULL,
        IoGetFileObjectGenericMapping(),
        Irp->RequestorMode,
        &GrantedAccess,
        &Status);
    SeReleaseSubjectContext(&SubjectContext);
    if (!HasAccess)
        goto cleanup;
    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case TUN_IOCTL_REGISTER_RINGS: {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&TunDispatchCtxGuard, TRUE);
#pragma warning(suppress : 28175)
        TUN_CTX *Ctx = DeviceObject->Reserved;
        Status = NDIS_STATUS_ADAPTER_NOT_READY;
        if (Ctx)
            Status = TunRegisterBuffers(Ctx, Irp);
        ExReleaseResourceLite(&TunDispatchCtxGuard);
        KeLeaveCriticalRegion();
        break;
    }
    case TUN_IOCTL_FORCE_CLOSE_HANDLES:
        Status = TunForceHandlesClosed(Stack->FileObject->DeviceObject) ? STATUS_SUCCESS : STATUS_NOTHING_TO_TERMINATE;
        break;
    }
cleanup:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

_Dispatch_type_(IRP_MJ_CLOSE)
static DRIVER_DISPATCH_PAGED TunDispatchClose;
_Use_decl_annotations_
static NTSTATUS
TunDispatchClose(DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&TunDispatchCtxGuard, TRUE);
#pragma warning(suppress : 28175)
    TUN_CTX *Ctx = DeviceObject->Reserved;
    if (Ctx)
        TunUnregisterBuffers(Ctx, IoGetCurrentIrpStackLocation(Irp)->FileObject);
    ExReleaseResourceLite(&TunDispatchCtxGuard);
    KeLeaveCriticalRegion();
    return NdisDispatchClose(DeviceObject, Irp);
}

static MINIPORT_RESTART TunRestart;
_Use_decl_annotations_
static NDIS_STATUS
TunRestart(NDIS_HANDLE MiniportAdapterContext, PNDIS_MINIPORT_RESTART_PARAMETERS MiniportRestartParameters)
{
    TUN_CTX *Ctx = (TUN_CTX *)MiniportAdapterContext;
    WriteRelease(&Ctx->Running, TRUE);
    return NDIS_STATUS_SUCCESS;
}

static MINIPORT_PAUSE TunPause;
_Use_decl_annotations_
static NDIS_STATUS
TunPause(NDIS_HANDLE MiniportAdapterContext, PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters)
{
    TUN_CTX *Ctx = (TUN_CTX *)MiniportAdapterContext;

    WriteRelease(&Ctx->Running, FALSE);
    ExReleaseSpinLockExclusive(
        &Ctx->TransitionLock,
        ExAcquireSpinLockExclusive(&Ctx->TransitionLock)); /* Ensure above change is visible to all readers. */

    KeWaitForSingleObject(&Ctx->Device.Receive.ActiveNbls.Empty, Executive, KernelMode, FALSE, NULL);

    return NDIS_STATUS_SUCCESS;
}

static MINIPORT_DEVICE_PNP_EVENT_NOTIFY TunDevicePnPEventNotify;
_Use_decl_annotations_
static VOID
TunDevicePnPEventNotify(NDIS_HANDLE MiniportAdapterContext, PNET_DEVICE_PNP_EVENT NetDevicePnPEvent)
{
}

static MINIPORT_INITIALIZE TunInitializeEx;
_Use_decl_annotations_
static NDIS_STATUS
TunInitializeEx(
    NDIS_HANDLE MiniportAdapterHandle,
    NDIS_HANDLE MiniportDriverContext,
    PNDIS_MINIPORT_INIT_PARAMETERS MiniportInitParameters)
{
    NDIS_STATUS Status;

    if (!MiniportAdapterHandle)
        return NDIS_STATUS_FAILURE;

/* Leaking memory 'Ctx'. Note: 'Ctx' is freed in TunHaltEx or on failure. */
#pragma warning(suppress : 6014)
    TUN_CTX *Ctx = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(*Ctx), TUN_MEMORY_TAG);
    if (!Ctx)
        return NDIS_STATUS_FAILURE;
    NdisZeroMemory(Ctx, sizeof(*Ctx));

    Ctx->MiniportAdapterHandle = MiniportAdapterHandle;

    NdisMGetDeviceProperty(MiniportAdapterHandle, NULL, &Ctx->FunctionalDeviceObject, NULL, NULL, NULL);
    if (Status = NDIS_STATUS_FAILURE, !Ctx->FunctionalDeviceObject)
        goto cleanupFreeCtx;
#pragma warning(suppress : 28175)
    ASSERT(!Ctx->FunctionalDeviceObject->Reserved);
    /* Reverse engineering indicates that we'd be better off calling
     * NdisWdfGetAdapterContextFromAdapterHandle(functional_device),
     * which points to our TUN_CTX object directly, but this isn't
     * available before Windows 10, so for now we just stick it into
     * this reserved field. Revisit this when we drop support for old
     * Windows versions. */
#pragma warning(suppress : 28175)
    Ctx->FunctionalDeviceObject->Reserved = Ctx;

    Ctx->Statistics.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    Ctx->Statistics.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    Ctx->Statistics.Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;
    Ctx->Statistics.SupportedStatistics =
        NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV | NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV |
        NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV | NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
        NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS | NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR |
        NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT | NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT |
        NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT | NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
        NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR | NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS |
        NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV | NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_RCV |
        NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_RCV | NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT |
        NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_XMIT | NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_XMIT;
    KeInitializeEvent(&Ctx->Device.Disconnected, NotificationEvent, TRUE);
    KeInitializeSpinLock(&Ctx->Device.Send.Lock);
    KeInitializeSpinLock(&Ctx->Device.Receive.Lock);
    KeInitializeEvent(&Ctx->Device.Receive.ActiveNbls.Empty, NotificationEvent, TRUE);
    ExInitializeResourceLite(&Ctx->Device.RegistrationLock);

    NET_BUFFER_LIST_POOL_PARAMETERS NblPoolParameters = {
        .Header = { .Type = NDIS_OBJECT_TYPE_DEFAULT,
                    .Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1,
                    .Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1 },
        .ProtocolId = NDIS_PROTOCOL_ID_DEFAULT,
        .fAllocateNetBuffer = TRUE,
        .PoolTag = TUN_MEMORY_TAG
    };
/* Leaking memory 'Ctx->NblPool'. Note: 'Ctx->NblPool' is freed in TunHaltEx or on failure. */
#pragma warning(suppress : 6014)
    Ctx->NblPool = NdisAllocateNetBufferListPool(MiniportAdapterHandle, &NblPoolParameters);
    if (Status = NDIS_STATUS_FAILURE, !Ctx->NblPool)
        goto cleanupFreeCtx;

    NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES AdapterRegistrationAttributes = {
        .Header = { .Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES,
                    .Revision = NdisVersion < NDIS_RUNTIME_VERSION_630
                                    ? NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1
                                    : NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2,
                    .Size = NdisVersion < NDIS_RUNTIME_VERSION_630
                                ? NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1
                                : NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2 },
        .AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_NO_HALT_ON_SUSPEND | NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK,
        .InterfaceType = NdisInterfaceInternal,
        .MiniportAdapterContext = Ctx
    };
    if (Status = NDIS_STATUS_FAILURE,
        !NT_SUCCESS(NdisMSetMiniportAttributes(
            MiniportAdapterHandle, (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterRegistrationAttributes)))
        goto cleanupFreeNblPool;

    NDIS_PM_CAPABILITIES PmCapabilities = {
        .Header = { .Type = NDIS_OBJECT_TYPE_DEFAULT,
                    .Revision = NdisVersion < NDIS_RUNTIME_VERSION_630 ? NDIS_PM_CAPABILITIES_REVISION_1
                                                                       : NDIS_PM_CAPABILITIES_REVISION_2,
                    .Size = NdisVersion < NDIS_RUNTIME_VERSION_630 ? NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1
                                                                   : NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2 },
        .MinMagicPacketWakeUp = NdisDeviceStateUnspecified,
        .MinPatternWakeUp = NdisDeviceStateUnspecified,
        .MinLinkChangeWakeUp = NdisDeviceStateUnspecified
    };
    static NDIS_OID SupportedOids[] = { OID_GEN_MAXIMUM_TOTAL_SIZE,
                                        OID_GEN_CURRENT_LOOKAHEAD,
                                        OID_GEN_TRANSMIT_BUFFER_SPACE,
                                        OID_GEN_RECEIVE_BUFFER_SPACE,
                                        OID_GEN_TRANSMIT_BLOCK_SIZE,
                                        OID_GEN_RECEIVE_BLOCK_SIZE,
                                        OID_GEN_VENDOR_DESCRIPTION,
                                        OID_GEN_VENDOR_ID,
                                        OID_GEN_VENDOR_DRIVER_VERSION,
                                        OID_GEN_XMIT_OK,
                                        OID_GEN_RCV_OK,
                                        OID_GEN_CURRENT_PACKET_FILTER,
                                        OID_GEN_STATISTICS,
                                        OID_GEN_INTERRUPT_MODERATION,
                                        OID_GEN_LINK_PARAMETERS,
                                        OID_PNP_SET_POWER,
                                        OID_PNP_QUERY_POWER };
    NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES AdapterGeneralAttributes = {
        .Header = { .Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES,
                    .Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2,
                    .Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2 },
        .MediaType = NdisMediumIP,
        .PhysicalMediumType = NdisPhysicalMediumUnspecified,
        .MtuSize = TUN_MAX_IP_PACKET_SIZE,
        .MaxXmitLinkSpeed = TUN_LINK_SPEED,
        .MaxRcvLinkSpeed = TUN_LINK_SPEED,
        .RcvLinkSpeed = TUN_LINK_SPEED,
        .XmitLinkSpeed = TUN_LINK_SPEED,
        .MediaConnectState = MediaConnectStateDisconnected,
        .LookaheadSize = TUN_MAX_IP_PACKET_SIZE,
        .MacOptions =
            NDIS_MAC_OPTION_TRANSFERS_NOT_PEND | NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | NDIS_MAC_OPTION_NO_LOOPBACK,
        .SupportedPacketFilters = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_ALL_MULTICAST |
                                  NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_ALL_LOCAL |
                                  NDIS_PACKET_TYPE_ALL_FUNCTIONAL,
        .AccessType = NET_IF_ACCESS_BROADCAST,
        .DirectionType = NET_IF_DIRECTION_SENDRECEIVE,
        .ConnectionType = NET_IF_CONNECTION_DEDICATED,
        .IfType = IF_TYPE_PROP_VIRTUAL,
        .IfConnectorPresent = FALSE,
        .SupportedStatistics = Ctx->Statistics.SupportedStatistics,
        .SupportedPauseFunctions = NdisPauseFunctionsUnsupported,
        .AutoNegotiationFlags =
            NDIS_LINK_STATE_XMIT_LINK_SPEED_AUTO_NEGOTIATED | NDIS_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED |
            NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED | NDIS_LINK_STATE_PAUSE_FUNCTIONS_AUTO_NEGOTIATED,
        .SupportedOidList = SupportedOids,
        .SupportedOidListLength = sizeof(SupportedOids),
        .PowerManagementCapabilitiesEx = &PmCapabilities
    };
    if (Status = NDIS_STATUS_FAILURE,
        !NT_SUCCESS(NdisMSetMiniportAttributes(
            MiniportAdapterHandle, (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterGeneralAttributes)))
        goto cleanupFreeNblPool;

    /* A miniport driver can call NdisMIndicateStatusEx after setting its
     * registration attributes even if the driver is still in the context
     * of the MiniportInitializeEx function. */
    TunIndicateStatus(Ctx->MiniportAdapterHandle, MediaConnectStateDisconnected);
    return NDIS_STATUS_SUCCESS;

cleanupFreeNblPool:
    NdisFreeNetBufferListPool(Ctx->NblPool);
cleanupFreeCtx:
    ExFreePoolWithTag(Ctx, TUN_MEMORY_TAG);
    return Status;
}

static MINIPORT_HALT TunHaltEx;
_Use_decl_annotations_
static VOID
TunHaltEx(NDIS_HANDLE MiniportAdapterContext, NDIS_HALT_ACTION HaltAction)
{
    TUN_CTX *Ctx = (TUN_CTX *)MiniportAdapterContext;

    TunUnregisterBuffers(Ctx, TUN_FORCE_UNREGISTRATION);

    ExReleaseSpinLockExclusive(
        &Ctx->TransitionLock,
        ExAcquireSpinLockExclusive(&Ctx->TransitionLock)); /* Ensure above change is visible to all readers. */
    NdisFreeNetBufferListPool(Ctx->NblPool);

#pragma warning(suppress : 6387)
    WritePointerNoFence(&Ctx->MiniportAdapterHandle, NULL);
#pragma warning(suppress : 6387 28175)
    WritePointerNoFence(&Ctx->FunctionalDeviceObject->Reserved, NULL);
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&TunDispatchCtxGuard, TRUE); /* Ensure above change is visible to all readers. */
    ExReleaseResourceLite(&TunDispatchCtxGuard);
    KeLeaveCriticalRegion();
    ExDeleteResourceLite(&Ctx->Device.RegistrationLock);
    ExFreePoolWithTag(Ctx, TUN_MEMORY_TAG);
}

static MINIPORT_SHUTDOWN TunShutdownEx;
_Use_decl_annotations_
static VOID
TunShutdownEx(NDIS_HANDLE MiniportAdapterContext, NDIS_SHUTDOWN_ACTION ShutdownAction)
{
}

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
static NDIS_STATUS
TunOidQueryWrite(_Inout_ NDIS_OID_REQUEST *OidRequest, _In_ ULONG Value)
{
    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof(ULONG))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof(ULONG);
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
    *(ULONG *)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer = Value;
    return NDIS_STATUS_SUCCESS;
}

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
static NDIS_STATUS
TunOidQueryWrite32or64(_Inout_ NDIS_OID_REQUEST *OidRequest, _In_ ULONG64 Value)
{
    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof(ULONG))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof(ULONG64);
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof(ULONG64))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof(ULONG64);
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
        *(ULONG *)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer = (ULONG)(Value & 0xffffffff);
        return NDIS_STATUS_SUCCESS;
    }

    OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG64);
    *(ULONG64 *)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer = Value;
    return NDIS_STATUS_SUCCESS;
}

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
static NDIS_STATUS
TunOidQueryWriteBuf(_Inout_ NDIS_OID_REQUEST *OidRequest, _In_bytecount_(Size) const void *Buf, _In_ ULONG Size)
{
    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < Size)
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = Size;
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = OidRequest->DATA.QUERY_INFORMATION.BytesWritten = Size;
    NdisMoveMemory(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer, Buf, Size);
    return NDIS_STATUS_SUCCESS;
}

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
static NDIS_STATUS
TunOidQuery(_Inout_ TUN_CTX *Ctx, _Inout_ NDIS_OID_REQUEST *OidRequest)
{
    ASSERT(
        OidRequest->RequestType == NdisRequestQueryInformation ||
        OidRequest->RequestType == NdisRequestQueryStatistics);

    switch (OidRequest->DATA.QUERY_INFORMATION.Oid)
    {
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        return TunOidQueryWrite(OidRequest, TUN_MAX_IP_PACKET_SIZE);

    case OID_GEN_TRANSMIT_BUFFER_SPACE:
        return TunOidQueryWrite(OidRequest, TUN_MAX_RING_CAPACITY);

    case OID_GEN_RECEIVE_BUFFER_SPACE:
        return TunOidQueryWrite(OidRequest, TUN_MAX_RING_CAPACITY);

    case OID_GEN_VENDOR_ID:
        return TunOidQueryWrite(OidRequest, HTONL(TUN_VENDOR_ID));

    case OID_GEN_VENDOR_DESCRIPTION:
        return TunOidQueryWriteBuf(OidRequest, TUN_VENDOR_NAME, (ULONG)sizeof(TUN_VENDOR_NAME));

    case OID_GEN_VENDOR_DRIVER_VERSION:
        return TunOidQueryWrite(OidRequest, (WINTUN_VERSION_MAJ << 16) | WINTUN_VERSION_MIN);

    case OID_GEN_XMIT_OK:
        return TunOidQueryWrite32or64(
            OidRequest,
            ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutUcastPkts) +
                ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutMulticastPkts) +
                ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCOutBroadcastPkts));

    case OID_GEN_RCV_OK:
        return TunOidQueryWrite32or64(
            OidRequest,
            ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCInUcastPkts) +
                ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCInMulticastPkts) +
                ReadNoFence64((LONG64 *)&Ctx->Statistics.ifHCInBroadcastPkts));

    case OID_GEN_STATISTICS:
        return TunOidQueryWriteBuf(OidRequest, &Ctx->Statistics, (ULONG)sizeof(Ctx->Statistics));

    case OID_GEN_INTERRUPT_MODERATION: {
        static const NDIS_INTERRUPT_MODERATION_PARAMETERS InterruptParameters = {
            .Header = { .Type = NDIS_OBJECT_TYPE_DEFAULT,
                        .Revision = NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1,
                        .Size = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1 },
            .InterruptModeration = NdisInterruptModerationNotSupported
        };
        return TunOidQueryWriteBuf(OidRequest, &InterruptParameters, (ULONG)sizeof(InterruptParameters));
    }

    case OID_PNP_QUERY_POWER:
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_SUCCESS;
    }

    OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
    return NDIS_STATUS_NOT_SUPPORTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
static NDIS_STATUS
TunOidSet(_Inout_ TUN_CTX *Ctx, _Inout_ NDIS_OID_REQUEST *OidRequest)
{
    ASSERT(OidRequest->RequestType == NdisRequestSetInformation);

    OidRequest->DATA.SET_INFORMATION.BytesNeeded = OidRequest->DATA.SET_INFORMATION.BytesRead = 0;

    switch (OidRequest->DATA.SET_INFORMATION.Oid)
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
    case OID_GEN_CURRENT_LOOKAHEAD:
        if (OidRequest->DATA.SET_INFORMATION.InformationBufferLength != 4)
        {
            OidRequest->DATA.SET_INFORMATION.BytesNeeded = 4;
            return NDIS_STATUS_INVALID_LENGTH;
        }
        OidRequest->DATA.SET_INFORMATION.BytesRead = 4;
        return NDIS_STATUS_SUCCESS;

    case OID_GEN_LINK_PARAMETERS:
        OidRequest->DATA.SET_INFORMATION.BytesRead = OidRequest->DATA.SET_INFORMATION.InformationBufferLength;
        return NDIS_STATUS_SUCCESS;

    case OID_GEN_INTERRUPT_MODERATION:
        return NDIS_STATUS_INVALID_DATA;

    case OID_PNP_SET_POWER:
        if (OidRequest->DATA.SET_INFORMATION.InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE))
        {
            OidRequest->DATA.SET_INFORMATION.BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
            return NDIS_STATUS_INVALID_LENGTH;
        }
        OidRequest->DATA.SET_INFORMATION.BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);
        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_NOT_SUPPORTED;
}

static MINIPORT_OID_REQUEST TunOidRequest;
_Use_decl_annotations_
static NDIS_STATUS
TunOidRequest(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST OidRequest)
{
    switch (OidRequest->RequestType)
    {
    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:
        return TunOidQuery(MiniportAdapterContext, OidRequest);

    case NdisRequestSetInformation:
        return TunOidSet(MiniportAdapterContext, OidRequest);

    default:
        return NDIS_STATUS_INVALID_OID;
    }
}

static MINIPORT_CANCEL_OID_REQUEST TunCancelOidRequest;
_Use_decl_annotations_
static VOID
TunCancelOidRequest(NDIS_HANDLE MiniportAdapterContext, PVOID RequestId)
{
}

static MINIPORT_DIRECT_OID_REQUEST TunDirectOidRequest;
_Use_decl_annotations_
static NDIS_STATUS
TunDirectOidRequest(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST OidRequest)
{
    switch (OidRequest->RequestType)
    {
    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:
    case NdisRequestSetInformation:
        return NDIS_STATUS_NOT_SUPPORTED;

    default:
        return NDIS_STATUS_INVALID_OID;
    }
}

static MINIPORT_CANCEL_DIRECT_OID_REQUEST TunCancelDirectOidRequest;
_Use_decl_annotations_
static VOID
TunCancelDirectOidRequest(NDIS_HANDLE MiniportAdapterContext, PVOID RequestId)
{
}

static MINIPORT_SYNCHRONOUS_OID_REQUEST TunSynchronousOidRequest;
_Use_decl_annotations_
static NDIS_STATUS
TunSynchronousOidRequest(NDIS_HANDLE MiniportAdapterContext, PNDIS_OID_REQUEST OidRequest)
{
    switch (OidRequest->RequestType)
    {
    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:
    case NdisRequestSetInformation:
        return NDIS_STATUS_NOT_SUPPORTED;

    default:
        return NDIS_STATUS_INVALID_OID;
    }
}

static MINIPORT_UNLOAD TunUnload;
_Use_decl_annotations_
static VOID
TunUnload(PDRIVER_OBJECT DriverObject)
{
    PsSetCreateProcessNotifyRoutine(TunProcessNotification, TRUE);
    NdisMDeregisterMiniportDriver(NdisMiniportDriverHandle);
    ExDeleteResourceLite(&TunDispatchCtxGuard);
    ExDeleteResourceLite(&TunDispatchDeviceListLock);
    ExFreePoolWithTag(TunDispatchSecurityDescriptor, TUN_MEMORY_TAG);
}

DRIVER_INITIALIZE DriverEntry;
_Use_decl_annotations_
NTSTATUS
DriverEntry(DRIVER_OBJECT *DriverObject, UNICODE_STRING *RegistryPath)
{
    NTSTATUS Status;

    if (!NT_SUCCESS(Status = TunInitializeDispatchSecurityDescriptor()))
        return Status;

    NdisVersion = NdisGetVersion();
    if (NdisVersion < NDIS_MINIPORT_VERSION_MIN)
        return NDIS_STATUS_UNSUPPORTED_REVISION;
    if (NdisVersion > NDIS_MINIPORT_VERSION_MAX)
        NdisVersion = NDIS_MINIPORT_VERSION_MAX;

    ExInitializeResourceLite(&TunDispatchCtxGuard);
    ExInitializeResourceLite(&TunDispatchDeviceListLock);

    NDIS_MINIPORT_DRIVER_CHARACTERISTICS miniport = {
        .Header = { .Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS,
                    .Revision = NdisVersion < NDIS_RUNTIME_VERSION_680
                                    ? NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2
                                    : NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_3,
                    .Size = NdisVersion < NDIS_RUNTIME_VERSION_680
                                ? NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2
                                : NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_3 },

        .MajorNdisVersion = (UCHAR)((NdisVersion & 0x00ff0000) >> 16),
        .MinorNdisVersion = (UCHAR)(NdisVersion & 0x000000ff),

        .MajorDriverVersion = WINTUN_VERSION_MAJ,
        .MinorDriverVersion = WINTUN_VERSION_MIN,

        .InitializeHandlerEx = TunInitializeEx,
        .HaltHandlerEx = TunHaltEx,
        .UnloadHandler = TunUnload,
        .PauseHandler = TunPause,
        .RestartHandler = TunRestart,
        .OidRequestHandler = TunOidRequest,
        .SendNetBufferListsHandler = TunSendNetBufferLists,
        .ReturnNetBufferListsHandler = TunReturnNetBufferLists,
        .CancelSendHandler = TunCancelSend,
        .DevicePnPEventNotifyHandler = TunDevicePnPEventNotify,
        .ShutdownHandlerEx = TunShutdownEx,
        .CancelOidRequestHandler = TunCancelOidRequest,
        .DirectOidRequestHandler = TunDirectOidRequest,
        .CancelDirectOidRequestHandler = TunCancelDirectOidRequest,
        .SynchronousOidRequestHandler = TunSynchronousOidRequest
    };

    Status = PsSetCreateProcessNotifyRoutine(TunProcessNotification, FALSE);
    if (!NT_SUCCESS(Status))
        goto cleanupResources;

    Status = NdisMRegisterMiniportDriver(DriverObject, RegistryPath, NULL, &miniport, &NdisMiniportDriverHandle);
    if (!NT_SUCCESS(Status))
        goto cleanupNotifier;

    NdisDispatchDeviceControl = DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    NdisDispatchClose = DriverObject->MajorFunction[IRP_MJ_CLOSE];
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TunDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = TunDispatchClose;

    return STATUS_SUCCESS;

cleanupNotifier:
    PsSetCreateProcessNotifyRoutine(TunProcessNotification, TRUE);
cleanupResources:
    ExDeleteResourceLite(&TunDispatchCtxGuard);
    ExDeleteResourceLite(&TunDispatchDeviceListLock);
    ExFreePoolWithTag(TunDispatchSecurityDescriptor, TUN_MEMORY_TAG);
    return Status;
}
