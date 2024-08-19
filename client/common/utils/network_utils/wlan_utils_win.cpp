#include "wlan_utils_win.h"

#include <Windows.h>

#include "../engine/engine/helper/helper_win.h"
#include "utils/logger.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

void *WlanUtils_win::helper_ = nullptr;

WlanUtils_win::WlanUtils_win() : loaded_(false)
{
    // This DLL is not available on default installs of Windows Server.  Dynamically load it so
    // the app doesn't fail to launch with a "DLL not found" error.  App profiling was performed
    // and indicated no performance degradation when dynamically loading and unloading the DLL.
    const QString dll = WinUtils::getSystemDir() + QString("\\wlanapi.dll");
    wlanDll_ = ::LoadLibraryEx(qUtf16Printable(dll), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (wlanDll_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: wlanapi.dll does not exist on this computer or could not be loaded:" << ::GetLastError();
        return;
    }

    pfnWlanOpenHandle_ = (WlanOpenHandleFunc)::GetProcAddress(wlanDll_, "WlanOpenHandle");
    if (pfnWlanOpenHandle_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: failed to load WlanOpenHandle:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanCloseHandle_ = (WlanCloseHandleFunc)::GetProcAddress(wlanDll_, "WlanCloseHandle");
    if (pfnWlanCloseHandle_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: failed to load WlanCloseHandle:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanFreeMemory_ = (WlanFreeMemoryFunc)::GetProcAddress(wlanDll_, "WlanFreeMemory");
    if (pfnWlanFreeMemory_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: failed to load WlanFreeMemory:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanEnumInterfaces_ = (WlanEnumInterfacesFunc)::GetProcAddress(wlanDll_, "WlanEnumInterfaces");
    if (pfnWlanEnumInterfaces_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: failed to load WlanEnumInterfaces:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanQueryInterface_ = (WlanQueryInterfaceFunc)::GetProcAddress(wlanDll_, "WlanQueryInterface");
    if (pfnWlanQueryInterface_ == NULL) {
        qCDebug(LOG_BASIC) << "WlanUtils_win: failed to load WlanQueryInterface:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    loaded_ = true;
}

WlanUtils_win::~WlanUtils_win()
{
    if (loaded_) {
        ::FreeLibrary(wlanDll_);
    }
}

QString WlanUtils_win::ssidFromInterfaceGUID(const QString &interfaceGUID)
{
    QString ssid;
    if (!loaded_) {
        qCDebug(LOG_BASIC) << "ssidFromInterfaceGUID called when wlanapi.dll is not loaded";
        return ssid;
    }

    DWORD dwCurVersion = 0;
    HANDLE hClient = NULL;
    auto result = pfnWlanOpenHandle_(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        qCDebug(LOG_BASIC) << "WlanOpenHandle failed with error:" << result;
        return ssid;
    }

    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;

    auto freeWlanResources = qScopeGuard([&] {
        if (pConnectInfo != NULL) {
            pfnWlanFreeMemory_(pConnectInfo);
        }

        pfnWlanCloseHandle_(hClient, NULL);
    });

    GUID actualGUID = WinUtils::stringToGuid(qPrintable(interfaceGUID));

    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

    result = pfnWlanQueryInterface_(hClient, &actualGUID, wlan_intf_opcode_current_connection, NULL,
                                    &connectInfoSize, (PVOID *) &pConnectInfo, &opCode);
    if (result != ERROR_SUCCESS) {
        if (result == ERROR_ACCESS_DENIED) {
            // Access to SSID information is blocked by the Location services security settings on this
            // computer.  Ask the helper to retrieve the SSID for us.
            return getSsidFromHelper(interfaceGUID);
        }

        // Will receive these error codes when an adapter is being reset.
        if (result != ERROR_NOT_FOUND && result != ERROR_INVALID_STATE) {
            qCDebug(LOG_BASIC) << "WlanQueryInterface failed with error:" << result;
        }

        return ssid;
    }

    std::string str_ssid;
    const auto &dot11Ssid = pConnectInfo->wlanAssociationAttributes.dot11Ssid;
    if (dot11Ssid.uSSIDLength != 0) {
        str_ssid.reserve(dot11Ssid.uSSIDLength);
        for (ULONG k = 0; k < dot11Ssid.uSSIDLength; k++) {
            str_ssid.push_back(static_cast<char>(dot11Ssid.ucSSID[k]));
        }
    }

    // Note: |str_ssid| can contain UTF-8 characters, but QString::fromStdString() can
    // handle the case.
    ssid = QString::fromStdString(str_ssid);

    return ssid;
}

bool WlanUtils_win::isWifiRadioOn()
{
    HANDLE hClient = NULL;
    DWORD dwCurVersion = 0;
    DWORD result = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;

    if (!loaded_) {
        qCDebug(LOG_BASIC) << "isWifiRadioOn called when wlanapi.dll is not loaded";
        return false;
    }

    result = pfnWlanOpenHandle_(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        qCDebug(LOG_BASIC) << "WlanOpenHandle failed with error:" << result;
        return false;
    }

    auto exitGuard = qScopeGuard([&] {
        if (pIfList) {
            pfnWlanFreeMemory_(pIfList);
        }
        pfnWlanCloseHandle_(hClient, NULL);
    });

    result = pfnWlanEnumInterfaces_(hClient, NULL, &pIfList);
    if (result != ERROR_SUCCESS) {
        qCDebug(LOG_BASIC) << "WlanEnumInterfaces failed with error:" << result;
        return false;
    }

    for (size_t i = 0; i < pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
        PWLAN_RADIO_STATE pRadioState = NULL;
        DWORD dwDataSize = sizeof(WLAN_RADIO_STATE);
        WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

        result = pfnWlanQueryInterface_(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_radio_state, NULL, &dwDataSize, (PVOID *)&pRadioState, &opCode);
        if (result != ERROR_SUCCESS) {
            qCDebug(LOG_BASIC) << "WlanQueryInterface failed with error: " << result;
            continue;
        }

        for (DWORD j = 0; j < pRadioState->dwNumberOfPhys; j++) {
            if (pRadioState->PhyRadioState[j].dot11SoftwareRadioState == dot11_radio_state_on &&
                pRadioState->PhyRadioState[j].dot11HardwareRadioState == dot11_radio_state_on)
            {
                pfnWlanFreeMemory_(pRadioState);
                return true;
            }
        }
        pfnWlanFreeMemory_(pRadioState);
    }

    return false;
}

void WlanUtils_win::setHelper(void *helper)
{
    helper_ = helper;
}

QString WlanUtils_win::getSsidFromHelper(const QString &interfaceGUID) const
{
    // To avoid spamming the log with this warning.
    static bool warnWLanAPIBlocked = true;

    WS_ASSERT(helper_ != nullptr);
    Helper_win *helper_win = static_cast<Helper_win*>(helper_);

    if (warnWLanAPIBlocked) {
        warnWLanAPIBlocked = false;
        qCDebug(LOG_BASIC) << "*** the WlanQueryInterface API is blocked on this computer";
    }

    QString ssid;
    DWORD result = helper_win->ssidFromInterfaceGUID(interfaceGUID, ssid);
    if (result != NO_ERROR && result != ERROR_NOT_FOUND && result != ERROR_INVALID_STATE) {
        // Will receive these error codes when an adapter is being reset.
        qCDebug(LOG_BASIC) << "getSsidFromHelper failed with error:" << result;
    }

    return ssid;
}
