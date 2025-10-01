/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2021 WireGuard LLC. All Rights Reserved.
 */

// Adapted from wireguard-windows-0.5.3\.deps\src\uapi\windows\wireguard.h
// to work with MSVC++ rather than llvm-mingw, in particular, how structure
// alignment is specified.

#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#define WG_KEY_LEN 32

typedef _declspec(align(8)) struct _WG_IOCTL_ALLOWED_IP
{
    union
    {
        IN_ADDR V4;
        IN6_ADDR V6;
    } Address;
        ADDRESS_FAMILY AddressFamily;
        UCHAR Cidr;
} WG_IOCTL_ALLOWED_IP;

typedef enum
{
    WG_IOCTL_PEER_HAS_PUBLIC_KEY = 1 << 0,
    WG_IOCTL_PEER_HAS_PRESHARED_KEY = 1 << 1,
    WG_IOCTL_PEER_HAS_PERSISTENT_KEEPALIVE = 1 << 2,
    WG_IOCTL_PEER_HAS_ENDPOINT = 1 << 3,
    WG_IOCTL_PEER_HAS_PROTOCOL_VERSION = 1 << 4,
    WG_IOCTL_PEER_REPLACE_ALLOWED_IPS = 1 << 5,
    WG_IOCTL_PEER_REMOVE = 1 << 6,
    WG_IOCTL_PEER_UPDATE = 1 << 7
} WG_IOCTL_PEER_FLAG;

typedef _declspec(align(8)) struct _WG_IOCTL_PEER
{
    WG_IOCTL_PEER_FLAG Flags;
    ULONG ProtocolVersion; /* 0 = latest protocol, 1 = this protocol. */
    UCHAR PublicKey[WG_KEY_LEN];
    UCHAR PresharedKey[WG_KEY_LEN];
    USHORT PersistentKeepalive;
    SOCKADDR_INET Endpoint;
    ULONG64 TxBytes;
    ULONG64 RxBytes;
    ULONG64 LastHandshake; /* Time of the last handshake, in 100ns intervals since 1601-01-01 UTC */
    ULONG AllowedIPsCount;
} WG_IOCTL_PEER;

typedef enum
{
    WG_IOCTL_INTERFACE_HAS_PUBLIC_KEY = 1 << 0,
    WG_IOCTL_INTERFACE_HAS_PRIVATE_KEY = 1 << 1,
    WG_IOCTL_INTERFACE_HAS_LISTEN_PORT = 1 << 2,
    WG_IOCTL_INTERFACE_REPLACE_PEERS = 1 << 3
} WG_IOCTL_INTERFACE_FLAG;

typedef _declspec(align(8)) struct _WG_IOCTL_INTERFACE
{
    WG_IOCTL_INTERFACE_FLAG Flags;
    USHORT ListenPort;
    UCHAR PrivateKey[WG_KEY_LEN];
    UCHAR PublicKey[WG_KEY_LEN];
    ULONG PeersCount;
} WG_IOCTL_INTERFACE;

#define WG_IOCTL_GET CTL_CODE(45208U, 321, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)
#define WG_IOCTL_SET CTL_CODE(45208U, 322, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

#define DEVPKEY_WG_NAME { \
        { 0x65726957, 0x7547, 0x7261, { 0x64, 0x4e, 0x61, 0x6d, 0x65, 0x4b, 0x65, 0x79 } }, \
        DEVPROPID_FIRST_USABLE + 1 \
    }
