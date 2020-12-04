/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

#include "adapter.h"
#include "elevate.h"
#include "entry.h"
#include "logger.h"
#include "namespace.h"
#include "nci.h"
#include "ntdll.h"
#include "registry.h"
#include "resource.h"
#include "wintun-inf.h"

#include <Windows.h>
#include <winternl.h>
#define _NTDEF_ /* TODO: figure out how to include ntsecapi and winternal together without requiring this */
#include <cfgmgr32.h>
#include <devguid.h>
#include <iphlpapi.h>
#include <ndisguid.h>
#include <NTSecAPI.h>
#include <SetupAPI.h>
#include <Shlwapi.h>
#include <wchar.h>
#include <initguid.h> /* Keep these two at bottom in this order, so that we only generate extra GUIDs for devpkey. The other keys we'll get from uuid.lib like usual. */
#include <devpkey.h>

#pragma warning(disable : 4221) /* nonstandard: address of automatic in initializer */

#define WAIT_FOR_REGISTRY_TIMEOUT 10000            /* ms */
#define MAX_POOL_DEVICE_TYPE (WINTUN_MAX_POOL + 8) /* Should accommodate a pool name with " Tunnel" appended */
#if defined(_M_IX86)
#    define IMAGE_FILE_PROCESS IMAGE_FILE_MACHINE_I386
#elif defined(_M_AMD64)
#    define IMAGE_FILE_PROCESS IMAGE_FILE_MACHINE_AMD64
#elif defined(_M_ARM)
#    define IMAGE_FILE_PROCESS IMAGE_FILE_MACHINE_ARMNT
#elif defined(_M_ARM64)
#    define IMAGE_FILE_PROCESS IMAGE_FILE_MACHINE_ARM64
#else
#    error Unsupported architecture
#endif

typedef struct _SP_DEVINFO_DATA_LIST
{
    SP_DEVINFO_DATA Data;
    struct _SP_DEVINFO_DATA_LIST *Next;
} SP_DEVINFO_DATA_LIST;

static USHORT NativeMachine = IMAGE_FILE_PROCESS;

static _Return_type_success_(return != NULL) SP_DRVINFO_DETAIL_DATA_W *GetAdapterDrvInfoDetail(
    _In_ HDEVINFO DevInfo,
    _In_opt_ SP_DEVINFO_DATA *DevInfoData,
    _In_ SP_DRVINFO_DATA_W *DrvInfoData)
{
    DWORD Size = sizeof(SP_DRVINFO_DETAIL_DATA_W) + 0x100;
    for (;;)
    {
        SP_DRVINFO_DETAIL_DATA_W *DrvInfoDetailData = Alloc(Size);
        if (!DrvInfoDetailData)
            return NULL;
        DrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_W);
        if (SetupDiGetDriverInfoDetailW(DevInfo, DevInfoData, DrvInfoData, DrvInfoDetailData, Size, &Size))
            return DrvInfoDetailData;
        DWORD LastError = GetLastError();
        Free(DrvInfoDetailData);
        if (LastError != ERROR_INSUFFICIENT_BUFFER)
        {
            SetLastError(LOG_ERROR(L"Failed", LastError));
            return NULL;
        }
    }
}

static _Return_type_success_(return != NULL) void *GetDeviceRegistryProperty(
    _In_ HDEVINFO DevInfo,
    _In_ SP_DEVINFO_DATA *DevInfoData,
    _In_ DWORD Property,
    _Out_opt_ DWORD *ValueType,
    _Inout_ DWORD *BufLen)
{
    for (;;)
    {
        BYTE *Data = Alloc(*BufLen);
        if (!Data)
            return NULL;
        if (SetupDiGetDeviceRegistryPropertyW(DevInfo, DevInfoData, Property, ValueType, Data, *BufLen, BufLen))
            return Data;
        DWORD LastError = GetLastError();
        Free(Data);
        if (LastError != ERROR_INSUFFICIENT_BUFFER)
        {
            SetLastError(LOG_ERROR(L"Querying property failed", LastError));
            return NULL;
        }
    }
}

static _Return_type_success_(return != NULL)
    WCHAR *GetDeviceRegistryString(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData, _In_ DWORD Property)
{
    DWORD LastError, ValueType, Size = 256 * sizeof(WCHAR);
    WCHAR *Buf = GetDeviceRegistryProperty(DevInfo, DevInfoData, Property, &ValueType, &Size);
    if (!Buf)
        return NULL;
    switch (ValueType)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        if (RegistryGetString(&Buf, Size / sizeof(WCHAR), ValueType))
            return Buf;
        LastError = GetLastError();
        break;
    default:
        LOG(WINTUN_LOG_ERR, L"Property is not a string");
        LastError = ERROR_INVALID_DATATYPE;
    }
    Free(Buf);
    SetLastError(LastError);
    return NULL;
}

static _Return_type_success_(return != NULL)
    WCHAR *GetDeviceRegistryMultiString(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData, _In_ DWORD Property)
{
    DWORD LastError, ValueType, Size = 256 * sizeof(WCHAR);
    WCHAR *Buf = GetDeviceRegistryProperty(DevInfo, DevInfoData, Property, &ValueType, &Size);
    if (!Buf)
        return NULL;
    switch (ValueType)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        if (RegistryGetMultiString(&Buf, Size / sizeof(WCHAR), ValueType))
            return Buf;
        LastError = GetLastError();
        break;
    default:
        LOG(WINTUN_LOG_ERR, L"Property is not a string");
        LastError = ERROR_INVALID_DATATYPE;
    }
    Free(Buf);
    SetLastError(LastError);
    return NULL;
}

static BOOL
IsOurHardwareID(_In_z_ const WCHAR *Hwids)
{
    for (; Hwids[0]; Hwids += wcslen(Hwids) + 1)
        if (!_wcsicmp(Hwids, WINTUN_HWID))
            return TRUE;
    return FALSE;
}

static BOOL
IsOurAdapter(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    WCHAR *Hwids = GetDeviceRegistryMultiString(DevInfo, DevInfoData, SPDRP_HARDWAREID);
    if (!Hwids)
    {
        LOG_LAST_ERROR(L"Failed to get hardware ID");
        return FALSE;
    }
    BOOL IsOurs = IsOurHardwareID(Hwids);
    Free(Hwids);
    return IsOurs;
}

static _Return_type_success_(return != INVALID_HANDLE_VALUE) HANDLE OpenDeviceObject(_In_opt_z_ const WCHAR *InstanceId)
{
    ULONG InterfacesLen;
    DWORD LastError = CM_MapCrToWin32Err(
        CM_Get_Device_Interface_List_SizeW(
            &InterfacesLen,
            (GUID *)&GUID_DEVINTERFACE_NET,
            (DEVINSTID_W)InstanceId,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT),
        ERROR_GEN_FAILURE);
    if (LastError != ERROR_SUCCESS)
    {
        SetLastError(LOG_ERROR(L"Failed to query associated instances size", LastError));
        return INVALID_HANDLE_VALUE;
    }
    WCHAR *Interfaces = Alloc(InterfacesLen * sizeof(WCHAR));
    if (!Interfaces)
        return INVALID_HANDLE_VALUE;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    LastError = CM_MapCrToWin32Err(
        CM_Get_Device_Interface_ListW(
            (GUID *)&GUID_DEVINTERFACE_NET,
            (DEVINSTID_W)InstanceId,
            Interfaces,
            InterfacesLen,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT),
        ERROR_GEN_FAILURE);
    if (LastError != ERROR_SUCCESS)
    {
        LOG_ERROR(L"Failed to get associated instances", LastError);
        goto cleanupBuf;
    }
    Handle = CreateFileW(
        Interfaces,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    LastError = Handle != INVALID_HANDLE_VALUE ? ERROR_SUCCESS : LOG_LAST_ERROR(L"Failed to connect to adapter");
cleanupBuf:
    Free(Interfaces);
    if (LastError != ERROR_SUCCESS)
        SetLastError(LastError);
    return Handle;
}

#define TUN_IOCTL_FORCE_CLOSE_HANDLES CTL_CODE(51820U, 0x971U, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

static _Return_type_success_(return != FALSE) BOOL
    ForceCloseWintunAdapterHandle(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    DWORD LastError = ERROR_SUCCESS;
    DWORD RequiredBytes;
    if (SetupDiGetDeviceInstanceIdW(DevInfo, DevInfoData, NULL, 0, &RequiredBytes) ||
        (LastError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        LOG_ERROR(L"Failed to query instance ID size", LastError);
        return FALSE;
    }
    WCHAR *InstanceId = Zalloc(sizeof(*InstanceId) * RequiredBytes);
    if (!InstanceId)
        return FALSE;
    if (!SetupDiGetDeviceInstanceIdW(DevInfo, DevInfoData, InstanceId, RequiredBytes, &RequiredBytes))
    {
        LastError = LOG_LAST_ERROR(L"Failed to get instance ID");
        goto cleanupInstanceId;
    }
    HANDLE NdisHandle = OpenDeviceObject(InstanceId);
    if (NdisHandle == INVALID_HANDLE_VALUE)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get adapter object");
        goto cleanupInstanceId;
    }
    if (DeviceIoControl(NdisHandle, TUN_IOCTL_FORCE_CLOSE_HANDLES, NULL, 0, NULL, 0, &RequiredBytes, NULL))
    {
        LastError = ERROR_SUCCESS;
        Sleep(200);
    }
    else if (GetLastError() == ERROR_NOTHING_TO_TERMINATE)
        LastError = ERROR_SUCCESS;
    else
        LastError = LOG_LAST_ERROR(L"Failed to perform ioctl");
    CloseHandle(NdisHandle);
cleanupInstanceId:
    Free(InstanceId);
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != FALSE) BOOL
    DisableAllOurAdapters(_In_ HDEVINFO DevInfo, _Inout_ SP_DEVINFO_DATA_LIST **DisabledAdapters)
{
    SP_PROPCHANGE_PARAMS Params = { .ClassInstallHeader = { .cbSize = sizeof(SP_CLASSINSTALL_HEADER),
                                                            .InstallFunction = DIF_PROPERTYCHANGE },
                                    .StateChange = DICS_DISABLE,
                                    .Scope = DICS_FLAG_GLOBAL };
    DWORD LastError = ERROR_SUCCESS;
    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        SP_DEVINFO_DATA_LIST *DeviceNode = Alloc(sizeof(SP_DEVINFO_DATA_LIST));
        if (!DeviceNode)
            return FALSE;
        DeviceNode->Data.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(DevInfo, EnumIndex, &DeviceNode->Data))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                Free(DeviceNode);
                break;
            }
            goto cleanupDeviceNode;
        }
        if (!IsOurAdapter(DevInfo, &DeviceNode->Data))
            goto cleanupDeviceNode;

        ULONG Status, ProblemCode;
        if (CM_Get_DevNode_Status(&Status, &ProblemCode, DeviceNode->Data.DevInst, 0) != CR_SUCCESS ||
            ((Status & DN_HAS_PROBLEM) && ProblemCode == CM_PROB_DISABLED))
            goto cleanupDeviceNode;

        LOG(WINTUN_LOG_INFO, L"Force closing all open handles for existing adapter");
        if (!ForceCloseWintunAdapterHandle(DevInfo, &DeviceNode->Data))
            LOG(WINTUN_LOG_WARN, L"Failed to force close adapter handles");

        LOG(WINTUN_LOG_INFO, L"Disabling existing adapter");
        if (!SetupDiSetClassInstallParamsW(DevInfo, &DeviceNode->Data, &Params.ClassInstallHeader, sizeof(Params)) ||
            !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DevInfo, &DeviceNode->Data))
        {
            LOG_LAST_ERROR(L"Failed to disable existing adapter");
            LastError = LastError != ERROR_SUCCESS ? LastError : GetLastError();
            goto cleanupDeviceNode;
        }

        DeviceNode->Next = *DisabledAdapters;
        *DisabledAdapters = DeviceNode;
        continue;

    cleanupDeviceNode:
        Free(DeviceNode);
    }
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != FALSE) BOOL
    EnableAllOurAdapters(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA_LIST *AdaptersToEnable)
{
    SP_PROPCHANGE_PARAMS Params = { .ClassInstallHeader = { .cbSize = sizeof(SP_CLASSINSTALL_HEADER),
                                                            .InstallFunction = DIF_PROPERTYCHANGE },
                                    .StateChange = DICS_ENABLE,
                                    .Scope = DICS_FLAG_GLOBAL };
    DWORD LastError = ERROR_SUCCESS;
    for (SP_DEVINFO_DATA_LIST *DeviceNode = AdaptersToEnable; DeviceNode; DeviceNode = DeviceNode->Next)
    {
        LOG(WINTUN_LOG_INFO, L"Enabling existing adapter");
        if (!SetupDiSetClassInstallParamsW(DevInfo, &DeviceNode->Data, &Params.ClassInstallHeader, sizeof(Params)) ||
            !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DevInfo, &DeviceNode->Data))
        {
            LOG_LAST_ERROR(L"Failed to enable existing adapter");
            LastError = LastError != ERROR_SUCCESS ? LastError : GetLastError();
        }
    }
    return RET_ERROR(TRUE, LastError);
}

void
AdapterInit(void)
{
    if (!MAYBE_WOW64)
        return;
    typedef BOOL(WINAPI * IsWow64Process2_t)(
        _In_ HANDLE hProcess, _Out_ USHORT * pProcessMachine, _Out_opt_ USHORT * pNativeMachine);
    HANDLE Kernel32;
    IsWow64Process2_t IsWow64Process2;
    USHORT ProcessMachine;
    if ((Kernel32 = GetModuleHandleW(L"kernel32.dll")) == NULL ||
        (IsWow64Process2 = (IsWow64Process2_t)GetProcAddress(Kernel32, "IsWow64Process2")) == NULL ||
        !IsWow64Process2(GetCurrentProcess(), &ProcessMachine, &NativeMachine))
    {
        BOOL IsWoW64;
        NativeMachine =
            IsWow64Process(GetCurrentProcess(), &IsWoW64) && IsWoW64 ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_PROCESS;
    }
}

static BOOL
CheckReboot(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    SP_DEVINSTALL_PARAMS_W DevInstallParams = { .cbSize = sizeof(SP_DEVINSTALL_PARAMS_W) };
    if (!SetupDiGetDeviceInstallParamsW(DevInfo, DevInfoData, &DevInstallParams))
    {
        LOG_LAST_ERROR(L"Retrieving device installation parameters failed");
        return FALSE;
    }
    SetLastError(ERROR_SUCCESS);
    return (DevInstallParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)) != 0;
}

static _Return_type_success_(return != FALSE) BOOL
    SetQuietInstall(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    SP_DEVINSTALL_PARAMS_W DevInstallParams = { .cbSize = sizeof(SP_DEVINSTALL_PARAMS_W) };
    if (!SetupDiGetDeviceInstallParamsW(DevInfo, DevInfoData, &DevInstallParams))
    {
        LOG_LAST_ERROR(L"Retrieving device installation parameters failed");
        return FALSE;
    }
    DevInstallParams.Flags |= DI_QUIETINSTALL;
    if (!SetupDiSetDeviceInstallParamsW(DevInfo, DevInfoData, &DevInstallParams))
    {
        LOG_LAST_ERROR(L"Setting device installation parameters failed");
        return FALSE;
    }
    return TRUE;
}

static _Return_type_success_(return != FALSE) BOOL
    GetNetCfgInstanceId(_In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData, _Out_ GUID *CfgInstanceID)
{
    HKEY Key = SetupDiOpenDevRegKey(DevInfo, DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_QUERY_VALUE);
    if (Key == INVALID_HANDLE_VALUE)
    {
        LOG_LAST_ERROR(L"Opening device registry key failed");
        return FALSE;
    }
    DWORD LastError = ERROR_SUCCESS;
    WCHAR *ValueStr = RegistryQueryString(Key, L"NetCfgInstanceId", TRUE);
    if (!ValueStr)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get NetCfgInstanceId");
        goto cleanupKey;
    }
    if (FAILED(CLSIDFromString(ValueStr, CfgInstanceID)))
    {
        LOG(WINTUN_LOG_ERR, L"NetCfgInstanceId is not a GUID");
        LastError = ERROR_INVALID_DATA;
    }
    Free(ValueStr);
cleanupKey:
    RegCloseKey(Key);
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != FALSE) BOOL
    GetDevInfoData(_In_ const GUID *CfgInstanceID, _Out_ HDEVINFO *DevInfo, _Out_ SP_DEVINFO_DATA *DevInfoData)
{
    *DevInfo = SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
    if (!*DevInfo)
    {
        LOG_LAST_ERROR(L"Failed to get present adapters");
        return FALSE;
    }
    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        DevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(*DevInfo, EnumIndex, DevInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }
        GUID CfgInstanceID2;
        if (GetNetCfgInstanceId(*DevInfo, DevInfoData, &CfgInstanceID2) &&
            !memcmp(CfgInstanceID, &CfgInstanceID2, sizeof(GUID)))
            return TRUE;
    }
    SetupDiDestroyDeviceInfoList(*DevInfo);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

static void
RemoveNumberedSuffix(_Inout_z_ WCHAR *Name)
{
    for (size_t i = wcslen(Name); i--;)
    {
        if ((Name[i] < L'0' || Name[i] > L'9') && !iswspace(Name[i]))
            return;
        Name[i] = 0;
    }
}

static _Return_type_success_(return != FALSE) BOOL
    GetPoolDeviceTypeName(_In_z_ const WCHAR *Pool, _Out_cap_c_(MAX_POOL_DEVICE_TYPE) WCHAR *Name)
{
    if (_snwprintf_s(Name, MAX_POOL_DEVICE_TYPE, _TRUNCATE, L"%s Tunnel", Pool) == -1)
    {
        LOG(WINTUN_LOG_ERR, L"Pool name too long");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

static BOOL
IsPoolMember(_In_z_ const WCHAR *Pool, _In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    WCHAR *DeviceDesc = GetDeviceRegistryString(DevInfo, DevInfoData, SPDRP_DEVICEDESC);
    WCHAR *FriendlyName = GetDeviceRegistryString(DevInfo, DevInfoData, SPDRP_FRIENDLYNAME);
    DWORD LastError = ERROR_SUCCESS;
    BOOL Ret = FALSE;
    WCHAR PoolDeviceTypeName[MAX_POOL_DEVICE_TYPE];
    if (!GetPoolDeviceTypeName(Pool, PoolDeviceTypeName))
    {
        LastError = GetLastError();
        goto cleanupNames;
    }
    Ret = (FriendlyName && !_wcsicmp(FriendlyName, PoolDeviceTypeName)) ||
          (DeviceDesc && !_wcsicmp(DeviceDesc, PoolDeviceTypeName));
    if (Ret)
        goto cleanupNames;
    if (FriendlyName)
        RemoveNumberedSuffix(FriendlyName);
    if (DeviceDesc)
        RemoveNumberedSuffix(DeviceDesc);
    Ret = (FriendlyName && !_wcsicmp(FriendlyName, PoolDeviceTypeName)) ||
          (DeviceDesc && !_wcsicmp(DeviceDesc, PoolDeviceTypeName));
cleanupNames:
    Free(FriendlyName);
    Free(DeviceDesc);
    SetLastError(LastError);
    return Ret;
}

static _Return_type_success_(return != NULL) WINTUN_ADAPTER
    *CreateAdapterData(_In_z_ const WCHAR *Pool, _In_ HDEVINFO DevInfo, _In_ SP_DEVINFO_DATA *DevInfoData)
{
    /* Open HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\<class>\<id> registry key. */
    HKEY Key = SetupDiOpenDevRegKey(DevInfo, DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_QUERY_VALUE);
    if (Key == INVALID_HANDLE_VALUE)
    {
        LOG_LAST_ERROR(L"Opening device registry key failed");
        return NULL;
    }

    DWORD LastError;
    WINTUN_ADAPTER *Adapter = Alloc(sizeof(WINTUN_ADAPTER));
    if (!Adapter)
    {
        LastError = GetLastError();
        goto cleanupKey;
    }

    WCHAR *ValueStr = RegistryQueryString(Key, L"NetCfgInstanceId", TRUE);
    if (!ValueStr)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get NetCfgInstanceId");
        goto cleanupAdapter;
    }
    if (FAILED(CLSIDFromString(ValueStr, &Adapter->CfgInstanceID)))
    {
        LOG(WINTUN_LOG_ERR, L"NetCfgInstanceId is not Adapter GUID");
        Free(ValueStr);
        LastError = ERROR_INVALID_DATA;
        goto cleanupAdapter;
    }
    Free(ValueStr);

    if (!RegistryQueryDWORD(Key, L"NetLuidIndex", &Adapter->LuidIndex, TRUE))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get NetLuidIndex");
        goto cleanupAdapter;
    }

    if (!RegistryQueryDWORD(Key, L"*IfType", &Adapter->IfType, TRUE))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get *IfType");
        goto cleanupAdapter;
    }

    DWORD Size;
    if (!SetupDiGetDeviceInstanceIdW(
            DevInfo, DevInfoData, Adapter->DevInstanceID, _countof(Adapter->DevInstanceID), &Size))
    {
        LastError = LOG_LAST_ERROR(L"Failed to get instance ID");
        goto cleanupAdapter;
    }

    if (wcsncpy_s(Adapter->Pool, _countof(Adapter->Pool), Pool, _TRUNCATE) == STRUNCATE)
    {
        LOG(WINTUN_LOG_ERR, L"Pool name too long");
        LastError = ERROR_INVALID_PARAMETER;
        goto cleanupAdapter;
    }
    RegCloseKey(Key);
    return Adapter;

cleanupAdapter:
    Free(Adapter);
cleanupKey:
    RegCloseKey(Key);
    SetLastError(LastError);
    return NULL;
}

static _Return_type_success_(return != FALSE) BOOL
    GetDeviceRegPath(_In_ const WINTUN_ADAPTER *Adapter, _Out_cap_c_(MAX_REG_PATH) WCHAR *Path)
{
    if (_snwprintf_s(
            Path,
            MAX_REG_PATH,
            _TRUNCATE,
            L"SYSTEM\\CurrentControlSet\\Enum\\%.*s",
            MAX_INSTANCE_ID,
            Adapter->DevInstanceID) == -1)
    {
        LOG(WINTUN_LOG_ERR, L"Registry path too long");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

void WINAPI
WintunFreeAdapter(_In_ WINTUN_ADAPTER *Adapter)
{
    Free(Adapter);
}

_Return_type_success_(return != NULL) WINTUN_ADAPTER *WINAPI
    WintunOpenAdapter(_In_z_ const WCHAR *Pool, _In_z_ const WCHAR *Name)
{
    if (!ElevateToSystem())
    {
        LOG(WINTUN_LOG_ERR, L"Failed to impersonate SYSTEM user");
        return NULL;
    }
    DWORD LastError;
    WINTUN_ADAPTER *Adapter = NULL;
    HANDLE Mutex = NamespaceTakePoolMutex(Pool);
    if (!Mutex)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        goto cleanupToken;
    }

    HDEVINFO DevInfo = SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
    if (DevInfo == INVALID_HANDLE_VALUE)
    {
        LastError = LOG_LAST_ERROR(L"Failed to get present adapters");
        goto cleanupMutex;
    }

    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        SP_DEVINFO_DATA DevInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
        if (!SetupDiEnumDeviceInfo(DevInfo, EnumIndex, &DevInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }

        GUID CfgInstanceID;
        if (!GetNetCfgInstanceId(DevInfo, &DevInfoData, &CfgInstanceID))
            continue;

        /* TODO: is there a better way than comparing ifnames? */
        WCHAR Name2[MAX_ADAPTER_NAME];
        if (NciGetConnectionName(&CfgInstanceID, Name2, sizeof(Name2), NULL) != ERROR_SUCCESS)
            continue;
        Name2[_countof(Name2) - 1] = 0;
        if (_wcsicmp(Name, Name2))
        {
            RemoveNumberedSuffix(Name2);
            if (_wcsicmp(Name, Name2))
                continue;
        }

        /* Check the Hardware ID to make sure it's a real Wintun device. */
        if (!IsOurAdapter(DevInfo, &DevInfoData))
        {
            LOG(WINTUN_LOG_ERR, L"Foreign adapter with the same name exists");
            LastError = ERROR_ALREADY_EXISTS;
            goto cleanupDevInfo;
        }

        if (!IsPoolMember(Pool, DevInfo, &DevInfoData))
        {
            if ((LastError = GetLastError()) == ERROR_SUCCESS)
            {
                LOG(WINTUN_LOG_ERR, L"Wintun adapter with the same name exists in another pool");
                LastError = ERROR_ALREADY_EXISTS;
                goto cleanupDevInfo;
            }
            else
            {
                LOG(WINTUN_LOG_ERR, L"Failed to get pool membership");
                goto cleanupDevInfo;
            }
        }

        Adapter = CreateAdapterData(Pool, DevInfo, &DevInfoData);
        LastError = Adapter ? ERROR_SUCCESS : LOG(WINTUN_LOG_ERR, L"Failed to create adapter data");
        goto cleanupDevInfo;
    }
    LastError = ERROR_FILE_NOT_FOUND;
cleanupDevInfo:
    SetupDiDestroyDeviceInfoList(DevInfo);
cleanupMutex:
    NamespaceReleaseMutex(Mutex);
cleanupToken:
    RevertToSelf();
    SetLastError(LastError);
    return Adapter;
}

_Return_type_success_(return != FALSE) BOOL WINAPI
    WintunGetAdapterName(_In_ const WINTUN_ADAPTER *Adapter, _Out_cap_c_(MAX_ADAPTER_NAME) WCHAR *Name)
{
    DWORD LastError = NciGetConnectionName(&Adapter->CfgInstanceID, Name, MAX_ADAPTER_NAME * sizeof(WCHAR), NULL);
    if (LastError != ERROR_SUCCESS)
    {
        SetLastError(LOG_ERROR(L"Failed to get name", LastError));
        return FALSE;
    }
    return TRUE;
}

static _Return_type_success_(return != FALSE) BOOL
    ConvertInterfaceAliasToGuid(_In_z_ const WCHAR *Name, _Out_ GUID *Guid)
{
    NET_LUID Luid;
    DWORD LastError = ConvertInterfaceAliasToLuid(Name, &Luid);
    if (LastError != NO_ERROR)
    {
        SetLastError(LOG_ERROR(L"Failed convert interface alias name to the locally unique identifier", LastError));
        return FALSE;
    }
    LastError = ConvertInterfaceLuidToGuid(&Luid, Guid);
    if (LastError != NO_ERROR)
    {
        SetLastError(LOG_ERROR(L"Failed convert interface locally to globally unique identifier", LastError));
        return FALSE;
    }
    return TRUE;
}

_Return_type_success_(return != FALSE) BOOL WINAPI
    WintunSetAdapterName(_In_ const WINTUN_ADAPTER *Adapter, _In_z_ const WCHAR *Name)
{
    DWORD LastError;
    const int MaxSuffix = 1000;
    WCHAR AvailableName[MAX_ADAPTER_NAME];
    if (wcsncpy_s(AvailableName, _countof(AvailableName), Name, _TRUNCATE) == STRUNCATE)
    {
        LOG(WINTUN_LOG_ERR, L"Adapter name too long");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    for (int i = 0;; ++i)
    {
        LastError = NciSetConnectionName(&Adapter->CfgInstanceID, AvailableName);
        if (LastError == ERROR_DUP_NAME)
        {
            GUID Guid2;
            if (ConvertInterfaceAliasToGuid(AvailableName, &Guid2))
            {
                for (int j = 0; j < MaxSuffix; ++j)
                {
                    WCHAR Proposal[MAX_ADAPTER_NAME];
                    if (_snwprintf_s(Proposal, _countof(Proposal), _TRUNCATE, L"%s %d", Name, j + 1) == -1)
                    {
                        LOG(WINTUN_LOG_ERR, L"Adapter name too long");
                        SetLastError(ERROR_INVALID_PARAMETER);
                        return FALSE;
                    }
                    if (_wcsnicmp(Proposal, AvailableName, MAX_ADAPTER_NAME) == 0)
                        continue;
                    DWORD LastError2 = NciSetConnectionName(&Guid2, Proposal);
                    if (LastError2 == ERROR_DUP_NAME)
                        continue;
                    if (LastError2 == ERROR_SUCCESS)
                    {
                        LastError = NciSetConnectionName(&Adapter->CfgInstanceID, AvailableName);
                        if (LastError == ERROR_SUCCESS)
                            break;
                    }
                    break;
                }
            }
        }
        if (LastError == ERROR_SUCCESS)
            break;
        if (i >= MaxSuffix || LastError != ERROR_DUP_NAME)
        {
            SetLastError(LOG_ERROR(L"Setting adapter name failed", LastError));
            return FALSE;
        }
        if (_snwprintf_s(AvailableName, _countof(AvailableName), _TRUNCATE, L"%s %d", Name, i + 1) == -1)
        {
            LOG(WINTUN_LOG_ERR, L"Adapter name too long");
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    /* TODO: This should use NetSetup2 so that it doesn't get unset. */
    HKEY DeviceRegKey;
    WCHAR DeviceRegPath[MAX_REG_PATH];
    if (!GetDeviceRegPath(Adapter, DeviceRegPath))
        return FALSE;
    LastError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, DeviceRegPath, 0, KEY_SET_VALUE, &DeviceRegKey);
    if (LastError != ERROR_SUCCESS)
    {
        SetLastError(LOG_ERROR(L"Failed to open registry key", LastError));
        return FALSE;
    }
    WCHAR PoolDeviceTypeName[MAX_POOL_DEVICE_TYPE];
    if (!GetPoolDeviceTypeName(Adapter->Pool, PoolDeviceTypeName))
    {
        LastError = GetLastError();
        goto cleanupDeviceRegKey;
    }
    LastError = RegSetKeyValueW(
        DeviceRegKey,
        NULL,
        L"FriendlyName",
        REG_SZ,
        PoolDeviceTypeName,
        (DWORD)((wcslen(PoolDeviceTypeName) + 1) * sizeof(WCHAR)));
    if (LastError != ERROR_SUCCESS)
        LOG_ERROR(L"Failed to set FriendlyName", LastError);
cleanupDeviceRegKey:
    RegCloseKey(DeviceRegKey);
    return RET_ERROR(TRUE, LastError);
}

void WINAPI
WintunGetAdapterLUID(_In_ const WINTUN_ADAPTER *Adapter, _Out_ NET_LUID *Luid)
{
    Luid->Info.Reserved = 0;
    Luid->Info.NetLuidIndex = Adapter->LuidIndex;
    Luid->Info.IfType = Adapter->IfType;
}

_Return_type_success_(return != INVALID_HANDLE_VALUE) HANDLE WINAPI
    AdapterOpenDeviceObject(_In_ const WINTUN_ADAPTER *Adapter)
{
    return OpenDeviceObject(Adapter->DevInstanceID);
}

static BOOL
HaveWHQL(void)
{
    if (HAVE_WHQL)
    {
        DWORD MajorVersion;
        RtlGetNtVersionNumbers(&MajorVersion, NULL, NULL);
        return MajorVersion >= 10;
    }
    return FALSE;
}

static _Return_type_success_(return != FALSE) BOOL InstallCertificate(_In_z_ const WCHAR *SignedResource)
{
    LOG(WINTUN_LOG_INFO, L"Trusting code signing certificate");
    DWORD SizeResource;
    const void *LockedResource = ResourceGetAddress(SignedResource, &SizeResource);
    if (!LockedResource)
    {
        LOG(WINTUN_LOG_ERR, L"Failed to locate resource");
        return FALSE;
    }
    const CERT_BLOB CertBlob = { .cbData = SizeResource, .pbData = (BYTE *)LockedResource };
    HCERTSTORE QueriedStore;
    if (!CryptQueryObject(
            CERT_QUERY_OBJECT_BLOB,
            &CertBlob,
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0,
            0,
            0,
            0,
            &QueriedStore,
            0,
            NULL))
    {
        LOG_LAST_ERROR(L"Failed to find certificate");
        return FALSE;
    }
    DWORD LastError = ERROR_SUCCESS;
    HCERTSTORE TrustedStore =
        CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"TrustedPublisher");
    if (!TrustedStore)
    {
        LastError = LOG_LAST_ERROR(L"Failed to open store");
        goto cleanupQueriedStore;
    }
    LPSTR CodeSigningOid[] = { szOID_PKIX_KP_CODE_SIGNING };
    CERT_ENHKEY_USAGE EnhancedUsage = { .cUsageIdentifier = 1, .rgpszUsageIdentifier = CodeSigningOid };
    for (const CERT_CONTEXT *CertContext = NULL; (CertContext = CertFindCertificateInStore(
                                                      QueriedStore,
                                                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                      CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                                      CERT_FIND_ENHKEY_USAGE,
                                                      &EnhancedUsage,
                                                      CertContext)) != NULL;)
    {
        CERT_EXTENSION *Ext = CertFindExtension(
            szOID_BASIC_CONSTRAINTS2, CertContext->pCertInfo->cExtension, CertContext->pCertInfo->rgExtension);
        CERT_BASIC_CONSTRAINTS2_INFO Constraints;
        DWORD Size = sizeof(Constraints);
        if (Ext &&
            CryptDecodeObjectEx(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                szOID_BASIC_CONSTRAINTS2,
                Ext->Value.pbData,
                Ext->Value.cbData,
                0,
                NULL,
                &Constraints,
                &Size) &&
            !Constraints.fCA)
            if (!CertAddCertificateContextToStore(TrustedStore, CertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL))
            {
                LOG_LAST_ERROR(L"Failed to add certificate to store");
                LastError = LastError != ERROR_SUCCESS ? LastError : GetLastError();
            }
    }
    CertCloseStore(TrustedStore, 0);
cleanupQueriedStore:
    CertCloseStore(QueriedStore, 0);
    return RET_ERROR(TRUE, LastError);
}

static BOOL
IsOurDrvInfoDetail(_In_ const SP_DRVINFO_DETAIL_DATA_W *DrvInfoDetailData)
{
    if (DrvInfoDetailData->CompatIDsOffset > 1 && !_wcsicmp(DrvInfoDetailData->HardwareID, WINTUN_HWID))
        return TRUE;
    if (DrvInfoDetailData->CompatIDsLength &&
        IsOurHardwareID(DrvInfoDetailData->HardwareID + DrvInfoDetailData->CompatIDsOffset))
        return TRUE;
    return FALSE;
}

static BOOL
IsNewer(
    _In_ const FILETIME *DriverDate1,
    _In_ DWORDLONG DriverVersion1,
    _In_ const FILETIME *DriverDate2,
    _In_ DWORDLONG DriverVersion2)
{
    if (DriverDate1->dwHighDateTime > DriverDate2->dwHighDateTime)
        return TRUE;
    if (DriverDate1->dwHighDateTime < DriverDate2->dwHighDateTime)
        return FALSE;

    if (DriverDate1->dwLowDateTime > DriverDate2->dwLowDateTime)
        return TRUE;
    if (DriverDate1->dwLowDateTime < DriverDate2->dwLowDateTime)
        return FALSE;

    if (DriverVersion1 > DriverVersion2)
        return TRUE;
    if (DriverVersion1 < DriverVersion2)
        return FALSE;

    return FALSE;
}

static _Return_type_success_(return != FALSE) BOOL
    GetTcpipAdapterRegPath(_In_ const WINTUN_ADAPTER *Adapter, _Out_cap_c_(MAX_REG_PATH) WCHAR *Path)
{
    WCHAR Guid[MAX_GUID_STRING_LEN];
    if (_snwprintf_s(
            Path,
            MAX_REG_PATH,
            _TRUNCATE,
            L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\%.*s",
            StringFromGUID2(&Adapter->CfgInstanceID, Guid, _countof(Guid)),
            Guid) == -1)
    {
        LOG(WINTUN_LOG_ERR, L"Registry path too long");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

static _Return_type_success_(return != FALSE) BOOL
    GetTcpipInterfaceRegPath(_In_ const WINTUN_ADAPTER *Adapter, _Out_cap_c_(MAX_REG_PATH) WCHAR *Path)
{
    HKEY TcpipAdapterRegKey;
    WCHAR TcpipAdapterRegPath[MAX_REG_PATH];
    if (!GetTcpipAdapterRegPath(Adapter, TcpipAdapterRegPath))
        return FALSE;
    DWORD LastError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TcpipAdapterRegPath, 0, KEY_QUERY_VALUE, &TcpipAdapterRegKey);
    if (LastError != ERROR_SUCCESS)
    {
        SetLastError(LOG_ERROR(L"Failed to open registry key", LastError));
        return FALSE;
    }
    WCHAR *Paths = RegistryQueryString(TcpipAdapterRegKey, L"IpConfig", TRUE);
    if (!Paths)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get IpConfig");
        goto cleanupTcpipAdapterRegKey;
    }
    if (!Paths[0])
    {
        LOG(WINTUN_LOG_ERR, L"IpConfig is empty");
        LastError = ERROR_INVALID_DATA;
        goto cleanupPaths;
    }
    if (_snwprintf_s(Path, MAX_REG_PATH, _TRUNCATE, L"SYSTEM\\CurrentControlSet\\Services\\%s", Paths) == -1)
    {
        LOG(WINTUN_LOG_ERR, L"Registry path too long");
        LastError = ERROR_INVALID_PARAMETER;
        goto cleanupPaths;
    }
cleanupPaths:
    Free(Paths);
cleanupTcpipAdapterRegKey:
    RegCloseKey(TcpipAdapterRegKey);
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != 0) DWORD VersionOfFile(_In_z_ const WCHAR *Filename)
{
    DWORD Zero;
    DWORD Len = GetFileVersionInfoSizeW(Filename, &Zero);
    if (!Len)
    {
        LOG_LAST_ERROR(L"Failed to query version info size");
        return 0;
    }
    VOID *VersionInfo = Alloc(Len);
    if (!VersionInfo)
        return 0;
    DWORD LastError = ERROR_SUCCESS, Version = 0;
    VS_FIXEDFILEINFO *FixedInfo;
    UINT FixedInfoLen = sizeof(*FixedInfo);
    if (!GetFileVersionInfoW(Filename, 0, Len, VersionInfo))
    {
        LastError = LOG_LAST_ERROR(L"Failed to get version info");
        goto out;
    }
    if (!VerQueryValueW(VersionInfo, L"\\", &FixedInfo, &FixedInfoLen))
    {
        LastError = LOG_LAST_ERROR(L"Failed to get version info root");
        goto out;
    }
    Version = FixedInfo->dwFileVersionMS;
    if (!Version)
    {
        LOG(WINTUN_LOG_WARN, L"Determined version of file, but was v0.0, so returning failure");
        LastError = ERROR_VERSION_PARSE_ERROR;
    }
out:
    Free(VersionInfo);
    return RET_ERROR(Version, LastError);
}

static _Return_type_success_(return != FALSE) BOOL
    CreateTemporaryDirectory(_Out_cap_c_(MAX_PATH) WCHAR *RandomTempSubDirectory)
{
    WCHAR WindowsDirectory[MAX_PATH];
    if (!GetWindowsDirectoryW(WindowsDirectory, _countof(WindowsDirectory)))
    {
        LOG_LAST_ERROR(L"Failed to get Windows folder");
        return FALSE;
    }
    WCHAR WindowsTempDirectory[MAX_PATH];
    if (!PathCombineW(WindowsTempDirectory, WindowsDirectory, L"Temp"))
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }
    UCHAR RandomBytes[32] = { 0 };
#pragma warning(suppress : 6387)
    if (!RtlGenRandom(RandomBytes, sizeof(RandomBytes)))
    {
        LOG(WINTUN_LOG_ERR, L"Failed to generate random");
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }
    WCHAR RandomSubDirectory[sizeof(RandomBytes) * 2 + 1];
    for (int i = 0; i < sizeof(RandomBytes); ++i)
        swprintf_s(&RandomSubDirectory[i * 2], 3, L"%02x", RandomBytes[i]);
    if (!PathCombineW(RandomTempSubDirectory, WindowsTempDirectory, RandomSubDirectory))
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }
    if (!CreateDirectoryW(RandomTempSubDirectory, &SecurityAttributes))
    {
        LOG_LAST_ERROR(L"Failed to create temporary folder");
        return FALSE;
    }
    return TRUE;
}

static DWORD WINAPI
MaybeGetRunningDriverVersion(BOOL ReturnOneIfRunningInsteadOfVersion)
{
    PRTL_PROCESS_MODULES Modules;
    ULONG BufferSize = 128 * 1024;
    for (;;)
    {
        Modules = Alloc(BufferSize);
        if (!Modules)
            return 0;
        NTSTATUS Status = NtQuerySystemInformation(SystemModuleInformation, Modules, BufferSize, &BufferSize);
        if (NT_SUCCESS(Status))
            break;
        Free(Modules);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            continue;
        LOG(WINTUN_LOG_ERR, L"Failed to enumerate drivers");
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }
    DWORD LastError = ERROR_SUCCESS, Version = 0;
    for (ULONG i = Modules->NumberOfModules; i-- > 0;)
    {
        const char *NtPath = (const char *)Modules->Modules[i].FullPathName;
        if (!_stricmp(&NtPath[Modules->Modules[i].OffsetToFileName], "wintun.sys"))
        {
            if (ReturnOneIfRunningInsteadOfVersion)
            {
                Version = 1;
                goto cleanupModules;
            }
            WCHAR FilePath[MAX_PATH * 3 + 15];
            if (_snwprintf_s(FilePath, _countof(FilePath), _TRUNCATE, L"\\\\?\\GLOBALROOT%S", NtPath) == -1)
                continue;
            Version = VersionOfFile(FilePath);
            if (!Version)
                LastError = GetLastError();
            goto cleanupModules;
        }
    }
    LastError = ERROR_FILE_NOT_FOUND;
cleanupModules:
    Free(Modules);
    return RET_ERROR(Version, LastError);
}

DWORD WINAPI
WintunGetRunningDriverVersion(void)
{
    return MaybeGetRunningDriverVersion(FALSE);
}

static BOOL
EnsureWintunUnloaded(void)
{
    BOOL Loaded;
    for (int i = 0; (Loaded = MaybeGetRunningDriverVersion(TRUE) != 0) != FALSE && i < 300; ++i)
        Sleep(50);
    return !Loaded;
}

static _Return_type_success_(return != FALSE) BOOL SelectDriver(
    _In_ HDEVINFO DevInfo,
    _In_opt_ SP_DEVINFO_DATA *DevInfoData,
    _Inout_ SP_DEVINSTALL_PARAMS_W *DevInstallParams)
{
    static const FILETIME OurDriverDate = WINTUN_INF_FILETIME;
    static const DWORDLONG OurDriverVersion = WINTUN_INF_VERSION;
    HANDLE DriverInstallationLock = NamespaceTakeDriverInstallationMutex();
    if (!DriverInstallationLock)
    {
        LOG(WINTUN_LOG_ERR, L"Failed to take driver installation mutex");
        return FALSE;
    }
    DWORD LastError;
    if (!SetupDiBuildDriverInfoList(DevInfo, DevInfoData, SPDIT_COMPATDRIVER))
    {
        LastError = LOG_LAST_ERROR(L"Failed building driver info list");
        goto cleanupDriverInstallationLock;
    }
    BOOL DestroyDriverInfoListOnCleanup = TRUE;
    FILETIME DriverDate = { 0 };
    DWORDLONG DriverVersion = 0;
    HDEVINFO DevInfoExistingAdapters = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA_LIST *ExistingAdapters = NULL;
    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        SP_DRVINFO_DATA_W DrvInfoData = { .cbSize = sizeof(SP_DRVINFO_DATA_W) };
        if (!SetupDiEnumDriverInfoW(DevInfo, DevInfoData, SPDIT_COMPATDRIVER, EnumIndex, &DrvInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }
        SP_DRVINFO_DETAIL_DATA_W *DrvInfoDetailData = GetAdapterDrvInfoDetail(DevInfo, DevInfoData, &DrvInfoData);
        if (!DrvInfoDetailData)
        {
            LOG(WINTUN_LOG_WARN, L"Failed getting driver info detail");
            continue;
        }
        if (!IsOurDrvInfoDetail(DrvInfoDetailData))
            goto next;
        if (IsNewer(&OurDriverDate, OurDriverVersion, &DrvInfoData.DriverDate, DrvInfoData.DriverVersion))
        {
            if (DevInfoExistingAdapters == INVALID_HANDLE_VALUE)
            {
                DevInfoExistingAdapters =
                    SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
                if (DevInfoExistingAdapters == INVALID_HANDLE_VALUE)
                {
                    LastError = LOG_LAST_ERROR(L"Failed to get present adapters");
                    Free(DrvInfoDetailData);
                    goto cleanupExistingAdapters;
                }
                _Analysis_assume_(DevInfoExistingAdapters != NULL);
                DisableAllOurAdapters(DevInfoExistingAdapters, &ExistingAdapters);
                LOG(WINTUN_LOG_INFO, L"Waiting for existing driver to unload from kernel");
                if (!EnsureWintunUnloaded())
                    LOG(WINTUN_LOG_WARN,
                        L"Failed to unload existing driver, which means a reboot will likely be required");
            }
            LOG(WINTUN_LOG_INFO, L"Removing existing driver");
            if (!SetupUninstallOEMInfW(PathFindFileNameW(DrvInfoDetailData->InfFileName), SUOI_FORCEDELETE, NULL))
                LOG_LAST_ERROR(L"Unable to remove existing driver");
            goto next;
        }
        if (!IsNewer(&DrvInfoData.DriverDate, DrvInfoData.DriverVersion, &DriverDate, DriverVersion))
            goto next;
        if (!SetupDiSetSelectedDriverW(DevInfo, DevInfoData, &DrvInfoData))
        {
            LOG_LAST_ERROR(L"Failed to select driver");
            goto next;
        }
        DriverDate = DrvInfoData.DriverDate;
        DriverVersion = DrvInfoData.DriverVersion;
    next:
        Free(DrvInfoDetailData);
    }

    if (DriverVersion)
    {
        LastError = ERROR_SUCCESS;
        DestroyDriverInfoListOnCleanup = FALSE;
        goto cleanupExistingAdapters;
    }

    WCHAR RandomTempSubDirectory[MAX_PATH];
    if (!CreateTemporaryDirectory(RandomTempSubDirectory))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to create temporary folder");
        goto cleanupExistingAdapters;
    }

    WCHAR CatPath[MAX_PATH] = { 0 };
    WCHAR SysPath[MAX_PATH] = { 0 };
    WCHAR InfPath[MAX_PATH] = { 0 };
    if (!PathCombineW(CatPath, RandomTempSubDirectory, L"wintun.cat") ||
        !PathCombineW(SysPath, RandomTempSubDirectory, L"wintun.sys") ||
        !PathCombineW(InfPath, RandomTempSubDirectory, L"wintun.inf"))
    {
        LastError = ERROR_BUFFER_OVERFLOW;
        goto cleanupDirectory;
    }

    BOOL UseWHQL = HaveWHQL();
    if (!UseWHQL && !InstallCertificate(L"wintun.cat"))
        LOG(WINTUN_LOG_WARN, L"Failed to install code signing certificate");

    LOG(WINTUN_LOG_INFO, L"Extracting driver");
    if (!ResourceCopyToFile(CatPath, UseWHQL ? L"wintun-whql.cat" : L"wintun.cat") ||
        !ResourceCopyToFile(SysPath, UseWHQL ? L"wintun-whql.sys" : L"wintun.sys") ||
        !ResourceCopyToFile(InfPath, UseWHQL ? L"wintun-whql.inf" : L"wintun.inf"))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to extract driver");
        goto cleanupDelete;
    }
    LOG(WINTUN_LOG_INFO, L"Installing driver");
    WCHAR InfStorePath[MAX_PATH];
    if (!SetupCopyOEMInfW(InfPath, NULL, SPOST_NONE, 0, InfStorePath, MAX_PATH, NULL, NULL))
    {
        LastError = LOG_LAST_ERROR(L"Could not install driver to store");
        goto cleanupDelete;
    }
    _Analysis_assume_nullterminated_(InfStorePath);

    SetupDiDestroyDriverInfoList(DevInfo, DevInfoData, SPDIT_COMPATDRIVER);
    DestroyDriverInfoListOnCleanup = FALSE;
    DevInstallParams->Flags |= DI_ENUMSINGLEINF;
    if (wcsncpy_s(DevInstallParams->DriverPath, _countof(DevInstallParams->DriverPath), InfStorePath, _TRUNCATE) ==
        STRUNCATE)
    {
        LOG(WINTUN_LOG_ERR, L"Inf path too long");
        LastError = ERROR_INVALID_PARAMETER;
        goto cleanupDelete;
    }
    if (!SetupDiSetDeviceInstallParamsW(DevInfo, DevInfoData, DevInstallParams))
    {
        LastError = LOG_LAST_ERROR(L"Setting device installation parameters failed");
        goto cleanupDelete;
    }
    if (!SetupDiBuildDriverInfoList(DevInfo, DevInfoData, SPDIT_COMPATDRIVER))
    {
        LastError = LOG_LAST_ERROR(L"Failed rebuilding driver info list");
        goto cleanupDelete;
    }
    DestroyDriverInfoListOnCleanup = TRUE;
    SP_DRVINFO_DATA_W DrvInfoData = { .cbSize = sizeof(SP_DRVINFO_DATA_W) };
    if (!SetupDiEnumDriverInfoW(DevInfo, DevInfoData, SPDIT_COMPATDRIVER, 0, &DrvInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Failed to get driver");
        goto cleanupDelete;
    }
    if (!SetupDiSetSelectedDriverW(DevInfo, DevInfoData, &DrvInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Failed to set driver");
        goto cleanupDelete;
    }
    LastError = ERROR_SUCCESS;
    DestroyDriverInfoListOnCleanup = FALSE;

cleanupDelete:
    DeleteFileW(CatPath);
    DeleteFileW(SysPath);
    DeleteFileW(InfPath);
cleanupDirectory:
    RemoveDirectoryW(RandomTempSubDirectory);
cleanupExistingAdapters:
    if (ExistingAdapters)
    {
        EnableAllOurAdapters(DevInfoExistingAdapters, ExistingAdapters);
        while (ExistingAdapters)
        {
            SP_DEVINFO_DATA_LIST *Next = ExistingAdapters->Next;
            Free(ExistingAdapters);
            ExistingAdapters = Next;
        }
    }
    if (DevInfoExistingAdapters != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(DevInfoExistingAdapters);
    if (DestroyDriverInfoListOnCleanup)
        SetupDiDestroyDriverInfoList(DevInfo, DevInfoData, SPDIT_COMPATDRIVER);
cleanupDriverInstallationLock:
    NamespaceReleaseMutex(DriverInstallationLock);
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != NULL) WINTUN_ADAPTER *CreateAdapter(
    _In_z_ const WCHAR *Pool,
    _In_z_ const WCHAR *Name,
    _In_opt_ const GUID *RequestedGUID,
    _Inout_ BOOL *RebootRequired)
{
    LOG(WINTUN_LOG_INFO, L"Creating adapter");

    HDEVINFO DevInfo = SetupDiCreateDeviceInfoListExW(&GUID_DEVCLASS_NET, NULL, NULL, NULL);
    if (DevInfo == INVALID_HANDLE_VALUE)
    {
        LOG_LAST_ERROR(L"Creating empty device information set failed");
        return NULL;
    }
    DWORD LastError;
    WINTUN_ADAPTER *Adapter = NULL;
    WCHAR ClassName[MAX_CLASS_NAME_LEN];
    if (!SetupDiClassNameFromGuidExW(&GUID_DEVCLASS_NET, ClassName, _countof(ClassName), NULL, NULL, NULL))
    {
        LastError = LOG_LAST_ERROR(L"Retrieving class name associated with class GUID failed");
        goto cleanupDevInfo;
    }

    WCHAR PoolDeviceTypeName[MAX_POOL_DEVICE_TYPE];
    if (!GetPoolDeviceTypeName(Pool, PoolDeviceTypeName))
    {
        LastError = GetLastError();
        goto cleanupDevInfo;
    }
    SP_DEVINFO_DATA DevInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
    if (!SetupDiCreateDeviceInfoW(
            DevInfo, ClassName, &GUID_DEVCLASS_NET, PoolDeviceTypeName, NULL, DICD_GENERATE_ID, &DevInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Creating new device information element failed");
        goto cleanupDevInfo;
    }
    SP_DEVINSTALL_PARAMS_W DevInstallParams = { .cbSize = sizeof(SP_DEVINSTALL_PARAMS_W) };
    if (!SetupDiGetDeviceInstallParamsW(DevInfo, &DevInfoData, &DevInstallParams))
    {
        LastError = LOG_LAST_ERROR(L"Retrieving device installation parameters failed");
        goto cleanupDevInfo;
    }
    DevInstallParams.Flags |= DI_QUIETINSTALL;
    if (!SetupDiSetDeviceInstallParamsW(DevInfo, &DevInfoData, &DevInstallParams))
    {
        LastError = LOG_LAST_ERROR(L"Setting device installation parameters failed");
        goto cleanupDevInfo;
    }
    if (!SetupDiSetSelectedDevice(DevInfo, &DevInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Failed selecting device");
        goto cleanupDevInfo;
    }
    static const WCHAR Hwids[_countof(WINTUN_HWID) + 1 /*Multi-string terminator*/] = WINTUN_HWID;
    if (!SetupDiSetDeviceRegistryPropertyW(DevInfo, &DevInfoData, SPDRP_HARDWAREID, (const BYTE *)Hwids, sizeof(Hwids)))
    {
        LastError = LOG_LAST_ERROR(L"Failed setting hardware ID");
        goto cleanupDevInfo;
    }

    if (!SelectDriver(DevInfo, &DevInfoData, &DevInstallParams))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to select driver");
        goto cleanupDevInfo;
    }

    HANDLE Mutex = NamespaceTakePoolMutex(Pool);
    if (!Mutex)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        goto cleanupDriverInfoList;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, DevInfo, &DevInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Registering device failed");
        goto cleanupDevice;
    }
    if (!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS, DevInfo, &DevInfoData))
        LOG_LAST_ERROR(L"Registering coinstallers failed");

    HKEY NetDevRegKey = INVALID_HANDLE_VALUE;
    const int PollTimeout = 50 /* ms */;
    for (int i = 0; NetDevRegKey == INVALID_HANDLE_VALUE && i < WAIT_FOR_REGISTRY_TIMEOUT / PollTimeout; ++i)
    {
        if (i)
            Sleep(PollTimeout);
        NetDevRegKey = SetupDiOpenDevRegKey(
            DevInfo, &DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_NOTIFY);
    }
    if (NetDevRegKey == INVALID_HANDLE_VALUE)
    {
        LastError = LOG_LAST_ERROR(L"Failed to open device-specific registry key");
        goto cleanupDevice;
    }
    if (RequestedGUID)
    {
        WCHAR RequestedGUIDStr[MAX_GUID_STRING_LEN];
        LastError = RegSetValueExW(
            NetDevRegKey,
            L"NetSetupAnticipatedInstanceId",
            0,
            REG_SZ,
            (const BYTE *)RequestedGUIDStr,
            StringFromGUID2(RequestedGUID, RequestedGUIDStr, _countof(RequestedGUIDStr)) * sizeof(WCHAR));
        if (LastError != ERROR_SUCCESS)
        {
            LOG_ERROR(L"Failed to set NetSetupAnticipatedInstanceId", LastError);
            goto cleanupNetDevRegKey;
        }
    }

    if (!SetupDiCallClassInstaller(DIF_INSTALLINTERFACES, DevInfo, &DevInfoData))
        LOG_LAST_ERROR(L"Installing interfaces failed");

    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICE, DevInfo, &DevInfoData))
    {
        LastError = LOG_LAST_ERROR(L"Installing device failed");
        goto cleanupNetDevRegKey;
    }
    *RebootRequired = *RebootRequired || CheckReboot(DevInfo, &DevInfoData);

    if (!SetupDiSetDeviceRegistryPropertyW(
            DevInfo,
            &DevInfoData,
            SPDRP_DEVICEDESC,
            (const BYTE *)PoolDeviceTypeName,
            (DWORD)((wcslen(PoolDeviceTypeName) + 1) * sizeof(WCHAR))))
    {
        LastError = LOG_LAST_ERROR(L"Failed to set adapter description");
        goto cleanupNetDevRegKey;
    }

    /* DIF_INSTALLDEVICE returns almost immediately, while the device installation continues in the background. It might
     * take a while, before all registry keys and values are populated. */
    WCHAR *DummyStr = RegistryQueryStringWait(NetDevRegKey, L"NetCfgInstanceId", WAIT_FOR_REGISTRY_TIMEOUT);
    if (!DummyStr)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get NetCfgInstanceId");
        goto cleanupNetDevRegKey;
    }
    Free(DummyStr);
    DWORD DummyDWORD;
    if (!RegistryQueryDWORDWait(NetDevRegKey, L"NetLuidIndex", WAIT_FOR_REGISTRY_TIMEOUT, &DummyDWORD))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get NetLuidIndex");
        goto cleanupNetDevRegKey;
    }
    if (!RegistryQueryDWORDWait(NetDevRegKey, L"*IfType", WAIT_FOR_REGISTRY_TIMEOUT, &DummyDWORD))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get *IfType");
        goto cleanupNetDevRegKey;
    }

    Adapter = CreateAdapterData(Pool, DevInfo, &DevInfoData);
    if (!Adapter)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to create adapter data");
        goto cleanupNetDevRegKey;
    }

    HKEY TcpipAdapterRegKey;
    WCHAR TcpipAdapterRegPath[MAX_REG_PATH];
    if (!GetTcpipAdapterRegPath(Adapter, TcpipAdapterRegPath))
    {
        LastError = GetLastError();
        goto cleanupAdapter;
    }
    TcpipAdapterRegKey = RegistryOpenKeyWait(
        HKEY_LOCAL_MACHINE, TcpipAdapterRegPath, KEY_QUERY_VALUE | KEY_NOTIFY, WAIT_FOR_REGISTRY_TIMEOUT);
    if (!TcpipAdapterRegKey)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to open adapter-specific TCP/IP interface registry key");
        goto cleanupAdapter;
    }
    DummyStr = RegistryQueryStringWait(TcpipAdapterRegKey, L"IpConfig", WAIT_FOR_REGISTRY_TIMEOUT);
    if (!DummyStr)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to get IpConfig");
        goto cleanupTcpipAdapterRegKey;
    }
    Free(DummyStr);

    WCHAR TcpipInterfaceRegPath[MAX_REG_PATH];
    if (!GetTcpipInterfaceRegPath(Adapter, TcpipInterfaceRegPath))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to determine interface-specific TCP/IP network registry key path");
        goto cleanupTcpipAdapterRegKey;
    }
    for (int Tries = 0; Tries < 300; ++Tries)
    {
        HKEY TcpipInterfaceRegKey = RegistryOpenKeyWait(
            HKEY_LOCAL_MACHINE, TcpipInterfaceRegPath, KEY_QUERY_VALUE | KEY_SET_VALUE, WAIT_FOR_REGISTRY_TIMEOUT);
        if (!TcpipInterfaceRegKey)
        {
            LastError = LOG(WINTUN_LOG_ERR, L"Failed to open interface-specific TCP/IP network registry key");
            goto cleanupTcpipAdapterRegKey;
        }

        static const DWORD EnableDeadGWDetect = 0;
        LastError = RegSetKeyValueW(
            TcpipInterfaceRegKey,
            NULL,
            L"EnableDeadGWDetect",
            REG_DWORD,
            &EnableDeadGWDetect,
            sizeof(EnableDeadGWDetect));
        RegCloseKey(TcpipInterfaceRegKey);
        if (LastError == ERROR_SUCCESS)
            break;
        if (LastError != ERROR_TRANSACTION_NOT_ACTIVE)
        {
            LOG_ERROR(L"Failed to set EnableDeadGWDetect", LastError);
            goto cleanupTcpipAdapterRegKey;
        }
        Sleep(10);
    }

    if (!WintunSetAdapterName(Adapter, Name))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to set adapter name");
        goto cleanupTcpipAdapterRegKey;
    }

    DEVPROPTYPE PropertyType;
    for (int Tries = 0; Tries < 1000; ++Tries)
    {
        NTSTATUS ProblemStatus;
        if (SetupDiGetDevicePropertyW(
                DevInfo,
                &DevInfoData,
                &DEVPKEY_Device_ProblemStatus,
                &PropertyType,
                (PBYTE)&ProblemStatus,
                sizeof(ProblemStatus),
                NULL,
                0) &&
            PropertyType == DEVPROP_TYPE_NTSTATUS)
        {
            LastError = RtlNtStatusToDosError(ProblemStatus);
            _Analysis_assume_(LastError != ERROR_SUCCESS);
            if (ProblemStatus != STATUS_PNP_DEVICE_CONFIGURATION_PENDING || Tries == 999)
            {
                LOG_ERROR(L"Failed to setup adapter", LastError);
                goto cleanupTcpipAdapterRegKey;
            }
            Sleep(10);
        }
        else
            break;
    }
    LastError = ERROR_SUCCESS;

cleanupTcpipAdapterRegKey:
    RegCloseKey(TcpipAdapterRegKey);
cleanupAdapter:
    if (LastError != ERROR_SUCCESS)
    {
        Free(Adapter);
        Adapter = NULL;
    }
cleanupNetDevRegKey:
    RegCloseKey(NetDevRegKey);
cleanupDevice:
    if (LastError != ERROR_SUCCESS)
    {
        /* The adapter failed to install, or the adapter ID was unobtainable. Clean-up. */
        SP_REMOVEDEVICE_PARAMS RemoveDeviceParams = { .ClassInstallHeader = { .cbSize = sizeof(SP_CLASSINSTALL_HEADER),
                                                                              .InstallFunction = DIF_REMOVE },
                                                      .Scope = DI_REMOVEDEVICE_GLOBAL };
        if (SetupDiSetClassInstallParamsW(
                DevInfo, &DevInfoData, &RemoveDeviceParams.ClassInstallHeader, sizeof(RemoveDeviceParams)) &&
            SetupDiCallClassInstaller(DIF_REMOVE, DevInfo, &DevInfoData))
            *RebootRequired = *RebootRequired || CheckReboot(DevInfo, &DevInfoData);
    }
    NamespaceReleaseMutex(Mutex);
cleanupDriverInfoList:
    SetupDiDestroyDriverInfoList(DevInfo, &DevInfoData, SPDIT_COMPATDRIVER);
cleanupDevInfo:
    SetupDiDestroyDeviceInfoList(DevInfo);
    return RET_ERROR(Adapter, LastError);
}

static _Return_type_success_(return != NULL)
    WINTUN_ADAPTER *GetAdapter(_In_z_ const WCHAR *Pool, _In_ const GUID *CfgInstanceID)
{
    HANDLE Mutex = NamespaceTakePoolMutex(Pool);
    if (!Mutex)
    {
        LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        return NULL;
    }
    DWORD LastError;
    WINTUN_ADAPTER *Adapter = NULL;
    HDEVINFO DevInfo;
    SP_DEVINFO_DATA DevInfoData;
    if (!GetDevInfoData(CfgInstanceID, &DevInfo, &DevInfoData))
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to locate adapter");
        goto cleanupMutex;
    }
    Adapter = CreateAdapterData(Pool, DevInfo, &DevInfoData);
    LastError = Adapter ? ERROR_SUCCESS : LOG(WINTUN_LOG_ERR, L"Failed to create adapter data");
    SetupDiDestroyDeviceInfoList(DevInfo);
cleanupMutex:
    NamespaceReleaseMutex(Mutex);
    return RET_ERROR(Adapter, LastError);
}

#include "rundll32_i.c"

_Return_type_success_(return != NULL) WINTUN_ADAPTER *WINAPI WintunCreateAdapter(
    _In_z_ const WCHAR *Pool,
    _In_z_ const WCHAR *Name,
    _In_opt_ const GUID *RequestedGUID,
    _Out_opt_ BOOL *RebootRequired)
{
    if (!ElevateToSystem())
    {
        LOG(WINTUN_LOG_ERR, L"Failed to impersonate SYSTEM user");
        return NULL;
    }
    BOOL DummyRebootRequired;
    if (!RebootRequired)
        RebootRequired = &DummyRebootRequired;
    *RebootRequired = FALSE;
    DWORD LastError;
    WINTUN_ADAPTER *Adapter;
    if (MAYBE_WOW64 && NativeMachine != IMAGE_FILE_PROCESS)
    {
        Adapter = CreateAdapterViaRundll32(Pool, Name, RequestedGUID, RebootRequired);
        LastError = Adapter ? ERROR_SUCCESS : GetLastError();
        goto cleanupToken;
    }
    Adapter = CreateAdapter(Pool, Name, RequestedGUID, RebootRequired);
    LastError = Adapter ? ERROR_SUCCESS : GetLastError();
cleanupToken:
    RevertToSelf();
    return RET_ERROR(Adapter, LastError);
}

_Return_type_success_(return != FALSE) BOOL WINAPI WintunDeleteAdapter(
    _In_ const WINTUN_ADAPTER *Adapter,
    _In_ BOOL ForceCloseSessions,
    _Out_opt_ BOOL *RebootRequired)
{
    if (!ElevateToSystem())
    {
        LOG(WINTUN_LOG_ERR, L"Failed to impersonate SYSTEM user");
        return FALSE;
    }
    BOOL DummyRebootRequired;
    if (!RebootRequired)
        RebootRequired = &DummyRebootRequired;
    *RebootRequired = FALSE;
    DWORD LastError;
    if (MAYBE_WOW64 && NativeMachine != IMAGE_FILE_PROCESS)
    {
        LastError =
            DeleteAdapterViaRundll32(Adapter, ForceCloseSessions, RebootRequired) ? ERROR_SUCCESS : GetLastError();
        goto cleanupToken;
    }

    HANDLE Mutex = NamespaceTakePoolMutex(Adapter->Pool);
    if (!Mutex)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        goto cleanupToken;
    }

    HDEVINFO DevInfo;
    SP_DEVINFO_DATA DevInfoData;
    if (!GetDevInfoData(&Adapter->CfgInstanceID, &DevInfo, &DevInfoData))
    {
        if ((LastError = GetLastError()) == ERROR_FILE_NOT_FOUND)
            LastError = ERROR_SUCCESS;
        else
            LOG(WINTUN_LOG_ERR, L"Failed to get adapter info data");
        goto cleanupMutex;
    }

    if (ForceCloseSessions && !ForceCloseWintunAdapterHandle(DevInfo, &DevInfoData))
        LOG(WINTUN_LOG_WARN, L"Failed to force close adapter handles");

    SetQuietInstall(DevInfo, &DevInfoData);
    SP_REMOVEDEVICE_PARAMS Params = { .ClassInstallHeader = { .cbSize = sizeof(SP_CLASSINSTALL_HEADER),
                                                              .InstallFunction = DIF_REMOVE },
                                      .Scope = DI_REMOVEDEVICE_GLOBAL };
    if ((!SetupDiSetClassInstallParamsW(DevInfo, &DevInfoData, &Params.ClassInstallHeader, sizeof(Params)) ||
         !SetupDiCallClassInstaller(DIF_REMOVE, DevInfo, &DevInfoData)) &&
        GetLastError() != ERROR_NO_SUCH_DEVINST)
    {
        LastError = LOG_LAST_ERROR(L"Failed to remove existing adapter");
        goto cleanupDevInfo;
    }
    LastError = ERROR_SUCCESS;
cleanupDevInfo:
    *RebootRequired = *RebootRequired || CheckReboot(DevInfo, &DevInfoData);
    SetupDiDestroyDeviceInfoList(DevInfo);
cleanupMutex:
    NamespaceReleaseMutex(Mutex);
cleanupToken:
    RevertToSelf();
    return RET_ERROR(TRUE, LastError);
}

static _Return_type_success_(return != FALSE) BOOL
    DeleteAllOurAdapters(_In_ const WCHAR Pool[WINTUN_MAX_POOL], _Inout_ BOOL *RebootRequired)
{
    HANDLE Mutex = NamespaceTakePoolMutex(Pool);
    if (!Mutex)
    {
        LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        return FALSE;
    }
    DWORD LastError = ERROR_SUCCESS;
    HDEVINFO DevInfo = SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
    if (DevInfo == INVALID_HANDLE_VALUE)
    {
        LastError = LOG_LAST_ERROR(L"Failed to get present adapters");
        goto cleanupMutex;
    }
    SP_REMOVEDEVICE_PARAMS Params = { .ClassInstallHeader = { .cbSize = sizeof(SP_CLASSINSTALL_HEADER),
                                                              .InstallFunction = DIF_REMOVE },
                                      .Scope = DI_REMOVEDEVICE_GLOBAL };
    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        SP_DEVINFO_DATA DevInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
        if (!SetupDiEnumDeviceInfo(DevInfo, EnumIndex, &DevInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }

        if (!IsOurAdapter(DevInfo, &DevInfoData) || !IsPoolMember(Pool, DevInfo, &DevInfoData))
            continue;

        LOG(WINTUN_LOG_INFO, L"Force closing all open handles for existing adapter");
        if (!ForceCloseWintunAdapterHandle(DevInfo, &DevInfoData))
            LOG(WINTUN_LOG_WARN, L"Failed to force close adapter handles");

        LOG(WINTUN_LOG_INFO, L"Removing existing adapter");
        if ((!SetupDiSetClassInstallParamsW(DevInfo, &DevInfoData, &Params.ClassInstallHeader, sizeof(Params)) ||
             !SetupDiCallClassInstaller(DIF_REMOVE, DevInfo, &DevInfoData)) &&
            GetLastError() != ERROR_NO_SUCH_DEVINST)
        {
            LOG_LAST_ERROR(L"Failed to remove existing adapter");
            LastError = LastError != ERROR_SUCCESS ? LastError : GetLastError();
        }
        *RebootRequired = *RebootRequired || CheckReboot(DevInfo, &DevInfoData);
    }
    SetupDiDestroyDeviceInfoList(DevInfo);
cleanupMutex:
    NamespaceReleaseMutex(Mutex);
    return RET_ERROR(TRUE, LastError);
}

_Return_type_success_(return != FALSE) BOOL WINAPI
    WintunDeletePoolDriver(_In_z_ const WCHAR *Pool, _Out_opt_ BOOL *RebootRequired)
{
    if (!ElevateToSystem())
    {
        LOG(WINTUN_LOG_ERR, L"Failed to impersonate SYSTEM user");
        return FALSE;
    }

    BOOL DummyRebootRequired;
    if (!RebootRequired)
        RebootRequired = &DummyRebootRequired;
    *RebootRequired = FALSE;

    DWORD LastError = ERROR_SUCCESS;
    if (MAYBE_WOW64 && NativeMachine != IMAGE_FILE_PROCESS)
    {
        LastError = DeletePoolDriverViaRundll32(Pool, RebootRequired) ? ERROR_SUCCESS : GetLastError();
        goto cleanupToken;
    }

    if (!DeleteAllOurAdapters(Pool, RebootRequired))
    {
        LastError = GetLastError();
        goto cleanupToken;
    }

    HANDLE DriverInstallationLock = NamespaceTakeDriverInstallationMutex();
    if (!DriverInstallationLock)
    {
        LastError = LOG(WINTUN_LOG_ERR, L"Failed to take driver installation mutex");
        goto cleanupToken;
    }
    HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVCLASS_NET, NULL, NULL, 0);
    if (!DeviceInfoSet)
    {
        LastError = LOG_LAST_ERROR(L"Failed to get adapter information");
        goto cleanupDriverInstallationLock;
    }
    if (!SetupDiBuildDriverInfoList(DeviceInfoSet, NULL, SPDIT_CLASSDRIVER))
    {
        LastError = LOG_LAST_ERROR(L"Failed building driver info list");
        goto cleanupDeviceInfoSet;
    }
    for (DWORD EnumIndex = 0;; ++EnumIndex)
    {
        SP_DRVINFO_DATA_W DriverInfo = { .cbSize = sizeof(DriverInfo) };
        if (!SetupDiEnumDriverInfoW(DeviceInfoSet, NULL, SPDIT_CLASSDRIVER, EnumIndex, &DriverInfo))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }
        SP_DRVINFO_DETAIL_DATA_W *DriverDetail = GetAdapterDrvInfoDetail(DeviceInfoSet, NULL, &DriverInfo);
        if (!DriverDetail)
            continue;
        if (!_wcsicmp(DriverDetail->HardwareID, WINTUN_HWID))
        {
            LOG(WINTUN_LOG_INFO, L"Removing existing driver");
            if (!SetupUninstallOEMInfW(PathFindFileNameW(DriverDetail->InfFileName), 0, NULL))
            {
                LOG_LAST_ERROR(L"Unable to remove existing driver");
                LastError = LastError != ERROR_SUCCESS ? LastError : GetLastError();
            }
        }
        Free(DriverDetail);
    }
    SetupDiDestroyDriverInfoList(DeviceInfoSet, NULL, SPDIT_CLASSDRIVER);
cleanupDeviceInfoSet:
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
cleanupDriverInstallationLock:
    NamespaceReleaseMutex(DriverInstallationLock);
cleanupToken:
    RevertToSelf();
    return RET_ERROR(TRUE, LastError);
}

_Return_type_success_(return != FALSE) BOOL WINAPI
    WintunEnumAdapters(_In_z_ const WCHAR *Pool, _In_ WINTUN_ENUM_CALLBACK Func, _In_ LPARAM Param)
{
    HANDLE Mutex = NamespaceTakePoolMutex(Pool);
    if (!Mutex)
    {
        LOG(WINTUN_LOG_ERR, L"Failed to take pool mutex");
        return FALSE;
    }
    DWORD LastError = ERROR_SUCCESS;
    HDEVINFO DevInfo = SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT, NULL, NULL, NULL);
    if (DevInfo == INVALID_HANDLE_VALUE)
    {
        LastError = LOG_LAST_ERROR(L"Failed to get present adapters");
        goto cleanupMutex;
    }
    BOOL Continue = TRUE;
    for (DWORD EnumIndex = 0; Continue; ++EnumIndex)
    {
        SP_DEVINFO_DATA DevInfoData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
        if (!SetupDiEnumDeviceInfo(DevInfo, EnumIndex, &DevInfoData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }

        if (!IsOurAdapter(DevInfo, &DevInfoData) || !IsPoolMember(Pool, DevInfo, &DevInfoData))
            continue;

        WINTUN_ADAPTER *Adapter = CreateAdapterData(Pool, DevInfo, &DevInfoData);
        if (!Adapter)
        {
            LastError = LOG(WINTUN_LOG_ERR, L"Failed to create adapter data");
            break;
        }
        Continue = Func(Adapter, Param);
        Free(Adapter);
    }
    SetupDiDestroyDeviceInfoList(DevInfo);
cleanupMutex:
    NamespaceReleaseMutex(Mutex);
    return RET_ERROR(TRUE, LastError);
}
