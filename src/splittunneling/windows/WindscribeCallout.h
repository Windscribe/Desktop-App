#pragma once

#include <initguid.h>

DEFINE_GUID(
    WINDSCRIBE_BIND_CALLOUT_GUID,
    0x5DF29179,
    0x344E,
    0x4F9C,
    0xA4, 0x5D, 0xC3, 0x0F, 0x95, 0x9B, 0x01, 0x2D
);

DEFINE_GUID(
    WINDSCRIBE_TCP_CALLOUT_GUID,
    0xB53C4ADE,
    0x7A35,
    0x4A63,
    0xB6, 0x90, 0xD9, 0x6B, 0xD4, 0x26, 0x23, 0x03
);

DEFINE_GUID(
    WINDSCRIBE_BIND_CALLOUT_V6_GUID,
    0x6EF39280,
    0x455F,
    0x4A9D,
    0xB5, 0x6E, 0xD4, 0x1F, 0xA6, 0xAC, 0x02, 0x36
);

DEFINE_GUID(
    WINDSCRIBE_TCP_CALLOUT_V6_GUID,
    0xC64D5BEF,
    0x8B46,
    0x4B74,
    0xC7, 0xA1, 0xEA, 0x7C, 0xE5, 0x37, 0x34, 0x14
);

#pragma pack(push,1)
typedef struct WINDSCRIBE_CALLOUT_DATA_
{
    UINT32 localIp;
    UINT32 vpnIp;
    UINT8 isExclude;

    // this is a list of addresses represented by pairs of addreses and masks:
    // excludeAddresses[0] -> Address_0; excludeAddresses[1] -> Mask_0
    // .....
    // excludeAddresses[cntExcludeAddresses - 2] -> Address_last; excludeAddresses[cntExcludeAddresses - 1] -> Mask_last
    UINT16 cntExcludeAddresses;
    UINT32 excludeAddresses[1];

} WINDSCRIBE_CALLOUT_DATA;

typedef struct WINDSCRIBE_CALLOUT_DATA_V6_
{
    UINT8 localIp[16];
    UINT8 vpnIp[16];
    UINT8 isExclude;

    // Pairs of 16-byte entries: address then mask (each IN6_ADDR-sized).
    // cntExcludeAddresses is the total number of 16-byte entries (pairs * 2).
    UINT16 cntExcludeAddresses;
    UINT8 excludeAddresses[1];

} WINDSCRIBE_CALLOUT_DATA_V6;
#pragma pack(pop)

