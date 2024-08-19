#pragma once

#include <QString>

#include <Windows.h>
#include <wlanapi.h>

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

    QString ssidFromInterfaceGUID(const QString &interfaceGUID);
    bool isWifiRadioOn();

    static void setHelper(void* helper);

private:
    HMODULE wlanDll_;
    bool loaded_;
    static void *helper_;

    WlanOpenHandleFunc pfnWlanOpenHandle_;
    WlanCloseHandleFunc pfnWlanCloseHandle_;
    WlanFreeMemoryFunc pfnWlanFreeMemory_;
    WlanEnumInterfacesFunc pfnWlanEnumInterfaces_;
    WlanQueryInterfaceFunc pfnWlanQueryInterface_;

    QString getSsidFromHelper(const QString &interfaceGUID) const;
};
