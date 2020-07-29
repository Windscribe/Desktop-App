/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2019 WireGuard LLC. All Rights Reserved.
 */

#include "installation.h"
#include <Windows.h>
#include <Msi.h>
#include <MsiQuery.h>
#include <tchar.h>

#pragma warning(disable : 4100) /* unreferenced formal parameter */

static MSIHANDLE MsiHandle;

#define ANCHOR_COMPONENT TEXT("{B668D4C7-ABB3-485A-B8DF-D34200489A43}")
#define PROCESS_ACTION TEXT("ProcessWintun")
#define ACTION_INSTALL TEXT("/WintunAction=Install")
#define ACTION_INSTALL_SEPERATOR TEXT('-')
#define ACTION_INSTALL_SEPERATORS TEXT("-%s-%s-%s")
#define ACTION_UNINSTALL TEXT("/WintunAction=Uninstall")
#define PROPERTY_INSTALLER_HASH TEXT("WintunInstallerHash")
#define PROPERTY_INSTALLER_BUILDTIME TEXT("WintunInstallerBuildtime")
#define PROPERTY_VERSION TEXT("WintunVersion")
#define REGKEY_WINTUN TEXT("Software\\Wintun")
#define REGKEY_INSTALLER_HASH TEXT("InstallerHash")
#define REGKEY_INSTALLER_BUILDTIME TEXT("InstallerBuildtime")
#define REGKEY_VERSION TEXT("Version")

static VOID
MsiLogger(_In_ LOGGER_LEVEL Level, _In_ const TCHAR *LogLine)
{
    MSIHANDLE Record = MsiCreateRecord(2);
    if (!Record)
        return;
    TCHAR *Template;
    INSTALLMESSAGE Type;
    switch (Level)
    {
    case LOG_INFO:
        Template = TEXT("Wintun: [1]");
        Type = INSTALLMESSAGE_INFO;
        break;
    case LOG_WARN:
        Template = TEXT("Wintun warning: [1]");
        Type = INSTALLMESSAGE_INFO;
        break;
    case LOG_ERR:
        Template = TEXT("Wintun error: [1]");
        Type = INSTALLMESSAGE_ERROR;
        break;
    default:
        goto cleanup;
    }
    MsiRecordSetString(Record, 0, Template);
    MsiRecordSetString(Record, 1, LogLine);
    MsiProcessMessage(MsiHandle, Type, Record);
cleanup:
    MsiCloseHandle(Record);
}

static BOOL
IsInstalling(_In_ INSTALLSTATE InstallState, _In_ INSTALLSTATE ActionState)
{
    return INSTALLSTATE_LOCAL == ActionState || INSTALLSTATE_SOURCE == ActionState ||
           (INSTALLSTATE_DEFAULT == ActionState &&
            (INSTALLSTATE_LOCAL == InstallState || INSTALLSTATE_SOURCE == InstallState));
}

static BOOL
IsReInstalling(_In_ INSTALLSTATE InstallState, _In_ INSTALLSTATE ActionState)
{
    return (INSTALLSTATE_LOCAL == ActionState || INSTALLSTATE_SOURCE == ActionState ||
            INSTALLSTATE_DEFAULT == ActionState) &&
           (INSTALLSTATE_LOCAL == InstallState || INSTALLSTATE_SOURCE == InstallState);
}

static BOOL
IsUninstalling(_In_ INSTALLSTATE InstallState, _In_ INSTALLSTATE ActionState)
{
    return (INSTALLSTATE_ABSENT == ActionState || INSTALLSTATE_REMOVED == ActionState) &&
           (INSTALLSTATE_LOCAL == InstallState || INSTALLSTATE_SOURCE == InstallState);
}

static UINT64
ParseVersion(_In_ const TCHAR *Version)
{
    ULONG Major = 0, Minor = 0, Revision = 0, Build = 0;
    _stscanf_s(Version, TEXT("%u.%u.%u.%u"), &Major, &Minor, &Revision, &Build);
    return ((UINT64)Major << 48) | ((UINT64)Minor << 32) | ((UINT64)Revision << 16) | ((UINT64)Build << 0);
}

_Success_(return )
static BOOL
Newer(_In_ MSIHANDLE Handle, _In_ BOOL SkipHashComparison, _Out_ TCHAR *InstallAction, _In_ SIZE_T InstallActionSize)
{
    INT64 NewTime, OldTime;
    UINT64 NewVersion, OldVersion;
    TCHAR NewHash[0x100], OldHash[0x100], NewTimeString[0x100], OldTimeString[0x100], NewVersionString[0x100],
        OldVersionString[0x100];
    DWORD Size, Type;
    HKEY Key;
    BOOL Ret = TRUE;

    Size = _countof(NewHash);
    if (MsiGetProperty(Handle, PROPERTY_INSTALLER_HASH, NewHash, &Size) != ERROR_SUCCESS)
        return FALSE;
    Size = _countof(NewTimeString);
    if (MsiGetProperty(Handle, PROPERTY_INSTALLER_BUILDTIME, NewTimeString, &Size) != ERROR_SUCCESS)
        return FALSE;
    NewTime = _tstoll(NewTimeString);
    Size = _countof(NewVersionString);
    if (MsiGetProperty(Handle, PROPERTY_VERSION, NewVersionString, &Size) != ERROR_SUCCESS)
        return FALSE;
    NewVersion = ParseVersion(NewVersionString);

    _stprintf_s(
        InstallAction,
        InstallActionSize,
        ACTION_INSTALL ACTION_INSTALL_SEPERATORS,
        NewHash,
        NewTimeString,
        NewVersionString);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WINTUN, 0, KEY_READ, &Key) != ERROR_SUCCESS)
        return TRUE;
    Size = sizeof(OldHash);
    if (RegQueryValueEx(Key, REGKEY_INSTALLER_HASH, NULL, &Type, (LPBYTE)OldHash, &Size) != ERROR_SUCCESS ||
        Type != REG_SZ)
        goto cleanup;
    Size = sizeof(OldTimeString);
    if (RegQueryValueEx(Key, REGKEY_INSTALLER_BUILDTIME, NULL, &Type, (LPBYTE)OldTimeString, &Size) != ERROR_SUCCESS ||
        Type != REG_SZ)
        goto cleanup;
    OldTime = _tstoll(OldTimeString);
    Size = sizeof(OldVersionString);
    if (RegQueryValueEx(Key, REGKEY_VERSION, NULL, &Type, (LPBYTE)OldVersionString, &Size) != ERROR_SUCCESS ||
        Type != REG_SZ)
        goto cleanup;
    OldVersion = ParseVersion(OldVersionString);

    Ret = NewVersion >= OldVersion && NewTime >= OldTime && (SkipHashComparison || _tcscmp(NewHash, OldHash));

cleanup:
    RegCloseKey(Key);
    return Ret;
}

UINT __stdcall MsiEvaluate(MSIHANDLE Handle)
{
    MsiHandle = Handle;
    SetLogger(MsiLogger);
    BOOL IsComInitialized = SUCCEEDED(CoInitialize(NULL));
    UINT Ret = ERROR_INSTALL_FAILURE;
    MSIHANDLE View = 0, Record = 0, Database = MsiGetActiveDatabase(Handle);
    if (!Database)
        goto cleanup;
    Ret = MsiDatabaseOpenView(
        Database, TEXT("SELECT `Component` FROM `Component` WHERE `ComponentId` = '" ANCHOR_COMPONENT "'"), &View);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;
    Ret = MsiViewExecute(View, 0);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;
    Ret = MsiViewFetch(View, &Record);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;
    TCHAR ComponentName[0x1000];
    DWORD Size = _countof(ComponentName);
    Ret = MsiRecordGetString(Record, 1, ComponentName, &Size);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;
    INSTALLSTATE InstallState, ActionState;
    Ret = MsiGetComponentState(Handle, ComponentName, &InstallState, &ActionState);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;
    TCHAR InstallAction[0x400];
    if ((IsReInstalling(InstallState, ActionState) || IsInstalling(InstallState, ActionState)) &&
        Newer(Handle, IsReInstalling(InstallState, ActionState), InstallAction, _countof(InstallAction)))
        Ret = MsiSetProperty(Handle, PROCESS_ACTION, InstallAction);
    else if (IsUninstalling(InstallState, ActionState))
        Ret = MsiSetProperty(Handle, PROCESS_ACTION, ACTION_UNINSTALL);
    if (Ret != ERROR_SUCCESS)
        goto cleanup;

    Ret = MsiDoAction(Handle, TEXT("DisableRollback"));

cleanup:
    if (View)
        MsiCloseHandle(View);
    if (Record)
        MsiCloseHandle(Record);
    if (Database)
        MsiCloseHandle(Database);
    if (IsComInitialized)
        CoUninitialize();
    return Ret;
}

static BOOL
WriteRegKeys(_In_ TCHAR *Values)
{
    TCHAR *Hash, *Time, *Version;
    Hash = Values;
    Time = _tcschr(Hash, ACTION_INSTALL_SEPERATOR);
    if (!Time)
        return FALSE;
    *Time++ = TEXT('\0');
    Version = _tcschr(Time, ACTION_INSTALL_SEPERATOR);
    if (!Version)
        return FALSE;
    *Version++ = TEXT('\0');

    HKEY Key;
    if (RegCreateKeyEx(
            HKEY_LOCAL_MACHINE, REGKEY_WINTUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &Key, NULL) !=
        ERROR_SUCCESS)
        return FALSE;
    BOOL Ret =
        RegSetValueEx(
            Key, REGKEY_INSTALLER_HASH, 0, REG_SZ, (LPBYTE)Hash, ((DWORD)_tcslen(Hash) + 1) * sizeof(*Hash)) ==
            ERROR_SUCCESS &&
        RegSetValueEx(
            Key, REGKEY_INSTALLER_BUILDTIME, 0, REG_SZ, (LPBYTE)Time, ((DWORD)_tcslen(Time) + 1) * sizeof(*Time)) ==
            ERROR_SUCCESS &&
        RegSetValueEx(
            Key, REGKEY_VERSION, 0, REG_SZ, (LPBYTE)Version, ((DWORD)_tcslen(Version) + 1) * sizeof(*Version)) ==
            ERROR_SUCCESS;
    RegCloseKey(Key);
    return Ret;
}

UINT __stdcall MsiProcess(MSIHANDLE Handle)
{
    MsiHandle = Handle;
    SetLogger(MsiLogger);
    BOOL IsComInitialized = SUCCEEDED(CoInitialize(NULL));
    DWORD LastError = ERROR_SUCCESS;
    BOOL Ret = FALSE;
    TCHAR Value[0x1000], *RegValues;
    DWORD Size = _countof(Value);
    LastError = MsiGetProperty(Handle, TEXT("CustomActionData"), Value, &Size);
    if (LastError != ERROR_SUCCESS)
        goto cleanup;
    if ((RegValues = _tcschr(Value, ACTION_INSTALL_SEPERATOR)) != NULL)
        *RegValues++ = TEXT('\0');
    if (!_tcscmp(Value, ACTION_INSTALL))
    {
        Ret = InstallOrUpdate();
        if (RegValues && Ret)
            Ret = WriteRegKeys(RegValues);
    }
    else if (!_tcscmp(Value, ACTION_UNINSTALL))
    {
        Ret = Uninstall();
        if (Ret)
            RegDeleteKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WINTUN, 0, 0);
    }
    else
        Ret = TRUE;
    LastError = GetLastError();
cleanup:
    if (IsComInitialized)
        CoUninitialize();
    return Ret ? ERROR_SUCCESS : LastError ? LastError : ERROR_INSTALL_FAILED;
}
