#include "wlan_utils_win.h"

#include <Windows.h>

#include "utils/log/logger.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

Helper *WlanUtils_win::helper_ = nullptr;

WlanUtils_win::WlanUtils_win() : loaded_(false)
{
    // This DLL is not available on default installs of Windows Server.  Dynamically load it so
    // the app doesn't fail to launch with a "DLL not found" error.  App profiling was performed
    // and indicated no performance degradation when dynamically loading and unloading the DLL.
    const QString dll = WinUtils::getSystemDir() + QString("\\wlanapi.dll");
    wlanDll_ = ::LoadLibraryEx(qUtf16Printable(dll), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (wlanDll_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: wlanapi.dll does not exist on this computer or could not be loaded:" << ::GetLastError();
        return;
    }

    pfnWlanOpenHandle_ = (WlanOpenHandleFunc)::GetProcAddress(wlanDll_, "WlanOpenHandle");
    if (pfnWlanOpenHandle_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: failed to load WlanOpenHandle:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanCloseHandle_ = (WlanCloseHandleFunc)::GetProcAddress(wlanDll_, "WlanCloseHandle");
    if (pfnWlanCloseHandle_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: failed to load WlanCloseHandle:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanFreeMemory_ = (WlanFreeMemoryFunc)::GetProcAddress(wlanDll_, "WlanFreeMemory");
    if (pfnWlanFreeMemory_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: failed to load WlanFreeMemory:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanEnumInterfaces_ = (WlanEnumInterfacesFunc)::GetProcAddress(wlanDll_, "WlanEnumInterfaces");
    if (pfnWlanEnumInterfaces_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: failed to load WlanEnumInterfaces:" << ::GetLastError();
        ::FreeLibrary(wlanDll_);
        return;
    }

    pfnWlanQueryInterface_ = (WlanQueryInterfaceFunc)::GetProcAddress(wlanDll_, "WlanQueryInterface");
    if (pfnWlanQueryInterface_ == NULL) {
        qCWarning(LOG_BASIC) << "WlanUtils_win: failed to load WlanQueryInterface:" << ::GetLastError();
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

DWORD WlanUtils_win::ssidFromInterfaceGUID(const QString &interfaceGUID, QString &ssid)
{
    if (!loaded_) {
        qCWarning(LOG_BASIC) << "ssidFromInterfaceGUID called when wlanapi.dll is not loaded";
        return ERROR_INVALID_STATE;
    }

    DWORD dwCurVersion = 0;
    HANDLE hClient = NULL;
    auto result = pfnWlanOpenHandle_(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        qCCritical(LOG_BASIC) << "WlanOpenHandle failed with error:" << result;
        return result;
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
            return getSsidFromHelper(interfaceGUID, ssid);
        }

        // Will receive these error codes when an adapter is being reset.
        if (result != ERROR_NOT_FOUND && result != ERROR_INVALID_STATE) {
            qCCritical(LOG_BASIC) << "WlanQueryInterface failed with error:" << result;
        }

        return result;
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

    return ERROR_SUCCESS;
}

bool WlanUtils_win::isWifiRadioOn()
{
    HANDLE hClient = NULL;
    DWORD dwCurVersion = 0;
    DWORD result = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;

    if (!loaded_) {
        qCWarning(LOG_BASIC) << "isWifiRadioOn called when wlanapi.dll is not loaded";
        return false;
    }

    result = pfnWlanOpenHandle_(2, NULL, &dwCurVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        qCCritical(LOG_BASIC) << "WlanOpenHandle failed with error:" << result;
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
        qCCritical(LOG_BASIC) << "WlanEnumInterfaces failed with error:" << result;
        return false;
    }

    for (size_t i = 0; i < pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
        PWLAN_RADIO_STATE pRadioState = NULL;
        DWORD dwDataSize = sizeof(WLAN_RADIO_STATE);
        WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

        result = pfnWlanQueryInterface_(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_radio_state, NULL, &dwDataSize, (PVOID *)&pRadioState, &opCode);
        if (result != ERROR_SUCCESS) {
            qCCritical(LOG_BASIC) << "WlanQueryInterface failed with error: " << result;
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

void WlanUtils_win::setHelper(Helper *helper)
{
    helper_ = helper;
}

DWORD WlanUtils_win::getSsidFromHelper(const QString &interfaceGUID, QString &out)
{
    // To avoid spamming the log with this warning.
    static bool warnWLanAPIBlocked = true;

    QString ssid;
    DWORD result = helper_->ssidFromInterfaceGUID(interfaceGUID, ssid);
    if (result == NO_ERROR) {
        // Reset this to cover the case where the user has enabled/disabled Location services while the
        // app is running.
        warnWLanAPIBlocked = true;
        out = ssid;
        return result;
    }

    if (result == ERROR_ACCESS_DENIED) {
        if (warnWLanAPIBlocked) {
            warnWLanAPIBlocked = false;
            qCWarning(LOG_BASIC) << "*** the WlanQueryInterface API is blocked on this computer. Windscribe will be unable to determine your Wi-Fi SSID until you enable Location services for Windows.";
        }
        return result;
    }

    else if (result != ERROR_NOT_FOUND && result != ERROR_INVALID_STATE) {
        // Will receive these error codes when an adapter is being reset.
        qCCritical(LOG_BASIC) << "getSsidFromHelper failed with error:" << result;
    }

    return result;
}
