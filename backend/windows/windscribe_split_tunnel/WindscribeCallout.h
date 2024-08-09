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
#pragma pack(pop)

