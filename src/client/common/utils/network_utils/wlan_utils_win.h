#pragma once

#include <QString>

#include <Windows.h>
#include <wlanapi.h>

#include "../engine/engine/helper/helper.h"

typedef DWORD (WINAPI * WlanOpenHandleFunc)(DWORD dwClientVersion, PVOID pReserved, PDWORD pdwNegotiatedVersion, PHANDLE phClientHandle);
typedef DWORD (WINAPI * WlanCloseHandleFunc)(HANDLE hClientHandle, PVOID pReserved);
typedef VOID  (WINAPI * WlanFreeMemoryFunc)(PVOID pMemory);
typedef DWORD (WINAPI * WlanEnumInterfacesFunc)(HANDLE hClientHandle, PVOID pReserved, PWLAN_INTERFACE_INFO_LIST *ppInterfaceList);
typedef DWORD (WINAPI * WlanQueryInterfaceFunc)(HANDLE hClientHandle, CONST GUID *pInterfaceGuid, WLAN_INTF_OPCODE OpCode, PVOID pReserved,
                                                PDWORD pdwDataSize, PVOID *ppData, PWLAN_OPCODE_VALUE_TYPE pWlanOpcodeValueType);

class WlanUtils_win
{
public:
    explicit WlanUtils_win();
    ~WlanUtils_win();

    DWORD ssidFromInterfaceGUID(const QString &interfaceGUID, QString &outSsid);
    bool isWifiRadioOn();

    static void setHelper(Helper* helper);

private:
    HMODULE wlanDll_;
    bool loaded_;
    static Helper *helper_;

    WlanOpenHandleFunc pfnWlanOpenHandle_;
    WlanCloseHandleFunc pfnWlanCloseHandle_;
    WlanFreeMemoryFunc pfnWlanFreeMemory_;
    WlanEnumInterfacesFunc pfnWlanEnumInterfaces_;
    WlanQueryInterfaceFunc pfnWlanQueryInterface_;

    DWORD getSsidFromHelper(const QString &interfaceGUID, QString &out);
};
