#pragma once

#include <initguid.h>

DEFINE_GUID(
    WINDSCRIBE_CALLOUT_GUID,
    0xB53C4ADE,
    0x7A35,
    0x4A63,
    0xB6, 0x90, 0xD9, 0x6B, 0xD4, 0x26, 0x21, 0x03
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
