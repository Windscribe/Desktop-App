#include "wlanmanager.h"
#include "Utils/logger.h"

WlanManager::WlanManager(QObject *parent) : QObject(parent),
    bDllFuncsLoaded_(false),
    wlanHandle_(NULL)
{
    HMODULE hModule = LoadLibrary(L"wlanapi.dll");
    if (hModule)
    {
        WlanOpenHandle_f = (WlanOpenHandle_T *)GetProcAddress(hModule, "WlanOpenHandle");
        if (!WlanOpenHandle_f) return;

        WlanRegisterNotification_f = (WlanRegisterNotification_T *)GetProcAddress(hModule, "WlanRegisterNotification");
        if (!WlanRegisterNotification_f) return;

        WlanHostedNetworkInitSettings_f = (WlanHostedNetworkInitSettings_T *)GetProcAddress(hModule, "WlanHostedNetworkInitSettings");
        if (!WlanHostedNetworkInitSettings_f) return;

        WlanHostedNetworkQueryStatus_f = (WlanHostedNetworkQueryStatus_T *)GetProcAddress(hModule, "WlanHostedNetworkQueryStatus");
        if (!WlanHostedNetworkQueryStatus_f) return;

        WlanFreeMemory_f = (WlanFreeMemory_T *)GetProcAddress(hModule, "WlanFreeMemory");
        if (!WlanFreeMemory_f) return;

        WlanHostedNetworkQueryProperty_f = (WlanHostedNetworkQueryProperty_T *)GetProcAddress(hModule, "WlanHostedNetworkQueryProperty");
        if (!WlanHostedNetworkQueryProperty_f) return;

        WlanCloseHandle_f = (WlanCloseHandle_T *)GetProcAddress(hModule, "WlanCloseHandle");
        if (!WlanCloseHandle_f) return;

        WlanHostedNetworkForceStop_f = (WlanHostedNetworkForceStop_T *)GetProcAddress(hModule, "WlanHostedNetworkForceStop");
        if (!WlanHostedNetworkForceStop_f) return;

        WlanHostedNetworkStopUsing_f = (WlanHostedNetworkStopUsing_T *)GetProcAddress(hModule, "WlanHostedNetworkStopUsing");
        if (!WlanHostedNetworkStopUsing_f) return;

        WlanHostedNetworkSetProperty_f = (WlanHostedNetworkSetProperty_T *)GetProcAddress(hModule, "WlanHostedNetworkSetProperty");
        if (!WlanHostedNetworkSetProperty_f) return;

        WlanHostedNetworkSetSecondaryKey_f = (WlanHostedNetworkSetSecondaryKey_T *)GetProcAddress(hModule, "WlanHostedNetworkSetSecondaryKey");
        if (!WlanHostedNetworkSetSecondaryKey_f) return;

        WlanHostedNetworkStartUsing_f = (WlanHostedNetworkStartUsing_T *)GetProcAddress(hModule, "WlanHostedNetworkStartUsing");
        if (!WlanHostedNetworkStartUsing_f) return;

        usersCounter_ = new ConnectedUsersCounter(this);
        connect(usersCounter_, SIGNAL(usersCountChanged()), SIGNAL(usersCountChanged()));

        bDllFuncsLoaded_ = true;
    }
}

WlanManager::~WlanManager()
{
    deinit();
}

bool WlanManager::isSupported()
{
    if (!bDllFuncsLoaded_)
    {
        return false;
    }

    HANDLE wlanHandle;
    DWORD serverVersion;
    DWORD retCode = ERROR_SUCCESS;
    DWORD dwDataSize = 0;
    BOOL *pbMode = NULL;
    WLAN_OPCODE_VALUE_TYPE valueType;

    retCode = WlanOpenHandle_f(WLAN_API_VERSION, NULL, &serverVersion, &wlanHandle);
    if (retCode != ERROR_SUCCESS)
    {
        return false;
    }

    retCode = WlanHostedNetworkInitSettings_f(wlanHandle, NULL, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }

    // check status
    WLAN_HOSTED_NETWORK_STATUS *pWlanHostedNetworkStatus = NULL;
    retCode = WlanHostedNetworkQueryStatus_f(wlanHandle, &pWlanHostedNetworkStatus, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }
    WlanFreeMemoryHelper pWlanHostedNetworkStatusStatus(pWlanHostedNetworkStatus, WlanFreeMemory_f);
    if (pWlanHostedNetworkStatus->HostedNetworkState == wlan_hosted_network_unavailable)
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }

    // Is hosted network enabled?
    retCode = WlanHostedNetworkQueryProperty_f(wlanHandle, wlan_hosted_network_opcode_enable,
                   &dwDataSize, (PVOID *)&pbMode, &valueType, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }

    if(!pbMode || dwDataSize < sizeof(BOOL))
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }
    WlanFreeMemoryHelper pbModeHelper(pbMode, WlanFreeMemory_f);
    if (*pbMode != TRUE)
    {
        WlanCloseHandle_f(wlanHandle, NULL);
        return false;
    }

    WlanCloseHandle_f(wlanHandle, NULL);
    return true;
}

bool WlanManager::init()
{
    if (!bDllFuncsLoaded_)
    {
        return false;
    }

    DWORD retCode = ERROR_SUCCESS;
    DWORD dwDataSize = 0;
    BOOL *pbMode = NULL;
    WLAN_OPCODE_VALUE_TYPE valueType;

    retCode = WlanOpenHandle_f(WLAN_API_VERSION, NULL, &serverVersion_, &wlanHandle_);
    if (retCode != ERROR_SUCCESS)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WlanOpenHandle failed: " << retCode;
        return false;
    }

    retCode = WlanRegisterNotification_f(wlanHandle_, WLAN_NOTIFICATION_SOURCE_HNWK, TRUE,
                                        &WlanManager::wlanNotificationCallback, this, NULL, NULL);

    if (retCode != ERROR_SUCCESS)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "WlanRegisterNotification failed: " << retCode;
        return false;
    }

    retCode = WlanHostedNetworkInitSettings_f(wlanHandle_, NULL, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkInitSettings failed: " << retCode;
        return false;
    }

    // check status
    WLAN_HOSTED_NETWORK_STATUS *pWlanHostedNetworkStatus = NULL;
    retCode = WlanHostedNetworkQueryStatus_f(wlanHandle_, &pWlanHostedNetworkStatus, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkQueryStatus failed: " << retCode;
        return false;
    }
    WlanFreeMemoryHelper pWlanHostedNetworkStatusStatus(pWlanHostedNetworkStatus, WlanFreeMemory_f);
    if (pWlanHostedNetworkStatus->HostedNetworkState == wlan_hosted_network_unavailable)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "HostedNetworkState == wlan_hosted_network_unavailable";
        return false;
    }

    // Is hosted network enabled?
    retCode = WlanHostedNetworkQueryProperty_f(wlanHandle_, wlan_hosted_network_opcode_enable,
                   &dwDataSize, (PVOID *)&pbMode, &valueType, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkQueryProperty failed: " << retCode;
        return false;
    }

    if(!pbMode || dwDataSize < sizeof(BOOL))
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkQueryProperty returned invalid data";
        return false;
    }
    WlanFreeMemoryHelper pbModeHelper(pbMode, WlanFreeMemory_f);
    if (*pbMode != TRUE)
    {
        safeCloseHandle();
        qCDebug(LOG_WLAN_MANAGER) << "The wireless Hosted Network is not allowed";
        return false;
    }

    return true;
}

void WlanManager::deinit()
{
    if (!bDllFuncsLoaded_)
    {
        return;
    }

    if (wlanHandle_)
    {
        DWORD retCode = ERROR_SUCCESS;
        WLAN_HOSTED_NETWORK_REASON failReason;
        retCode = WlanHostedNetworkForceStop_f(wlanHandle_, &failReason, NULL);
    }
    safeCloseHandle();
    usersCounter_->reset();
}

bool WlanManager::start(const QString &ssid, const QString &password)
{
    if (!bDllFuncsLoaded_)
    {
        return false;
    }

    ssid_ = ssid;
    password_ = password;

    DWORD retCode = ERROR_SUCCESS;
    WLAN_HOSTED_NETWORK_REASON failReason;
    WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS connectionSettings = {0};
    connectionSettings.dwMaxNumberOfPeers = 10;
    connectionSettings.hostedNetworkSSID.uSSIDLength = ssid.toStdString().size();
    memcpy(connectionSettings.hostedNetworkSSID.ucSSID, ssid.toStdString().c_str(), ssid.toStdString().size());
    retCode = WlanHostedNetworkSetProperty_f(wlanHandle_, wlan_hosted_network_opcode_connection_settings,
                                 sizeof(connectionSettings), &connectionSettings, &failReason, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkSetProperty failed: " << retCode << "; failReason:" << failReason;
        return false;
    }

    UCHAR *keyData = new UCHAR[password.toStdString().length() + 1];
    memcpy(keyData, password.toStdString().c_str(), password.toStdString().length());
    keyData[password.toStdString().length()] = '\0';
    retCode = WlanHostedNetworkSetSecondaryKey_f(wlanHandle_, password.toStdString().length() + 1, keyData, TRUE,
                                     FALSE, &failReason, NULL);
    delete[] keyData;

    if (retCode != ERROR_SUCCESS)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkSetSecondaryKey failed: " << retCode << "; failReason:" << failReason;
        return false;
    }
    retCode = WlanHostedNetworkForceStop_f(wlanHandle_, &failReason, NULL);
    retCode = WlanHostedNetworkStartUsing_f(wlanHandle_, &failReason, NULL);
    if (retCode != ERROR_SUCCESS)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkStartUsing failed: " << retCode << "; failReason:" << failReason;
        return false;
    }

    return true;
}

void WlanManager::getHostedNetworkInterfaceGuid(GUID &interfaceGuid)
{
    QMutexLocker locker(&mutex_);
    interfaceGuid = interfaceGuid_;
}

int WlanManager::getConnectedUsersCount()
{
    return usersCounter_->getConnectedUsersCount();
}

VOID WINAPI WlanManager::wlanNotificationCallback(PWLAN_NOTIFICATION_DATA pNotifData, PVOID pContext)
{
    WlanManager* this_ = (WlanManager *)pContext;

    if (!pNotifData)
    {
        return;
    }

    if (pNotifData->NotificationCode == wlan_hosted_network_state_change)
    {
        qCDebug(LOG_WLAN_MANAGER) << this_->makeDebugString_wlan_hosted_network_state_change(pNotifData);
        WLAN_HOSTED_NETWORK_STATE_CHANGE *stateChange = (WLAN_HOSTED_NETWORK_STATE_CHANGE *)pNotifData->pData;
        if (stateChange->NewState == wlan_hosted_network_active)
        {
            this_->updateHostedNetworkInterfaceGuid();
            emit this_->wlanStarted();
        }
        else if (stateChange->NewState == wlan_hosted_network_idle)
        {
            WLAN_HOSTED_NETWORK_STATE_CHANGE *stateChange = (WLAN_HOSTED_NETWORK_STATE_CHANGE *)pNotifData->pData;
            // for recovery when goto from sleep mode
            if (stateChange->StateChangeReason == wlan_hosted_network_reason_interface_available)
            {
                WLAN_HOSTED_NETWORK_REASON failReason;

                //set password again, otherwise password incorrect
                UCHAR *keyData = new UCHAR[this_->password_.toStdString().length() + 1];
                memcpy(keyData, this_->password_.toStdString().c_str(), this_->password_.toStdString().length());
                keyData[this_->password_.toStdString().length()] = '\0';
                this_->WlanHostedNetworkSetSecondaryKey_f(this_->wlanHandle_, this_->password_.toStdString().length() + 1, keyData, TRUE,
                                                 FALSE, &failReason, NULL);
                delete[] keyData;

                DWORD retCode = this_->WlanHostedNetworkStartUsing_f(this_->wlanHandle_, &failReason, NULL);
                if (retCode != ERROR_SUCCESS)
                {
                    qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkStartUsing failed: " << retCode << "; failReason:" << failReason;
                }
            }
        }
    }
    else if (pNotifData->NotificationCode == wlan_hosted_network_peer_state_change)
    {
        WLAN_HOSTED_NETWORK_DATA_PEER_STATE_CHANGE *pPeerStateChange = (WLAN_HOSTED_NETWORK_DATA_PEER_STATE_CHANGE *)pNotifData->pData;
        if (wlan_hosted_network_peer_state_authenticated == pPeerStateChange->NewState.PeerAuthState)
        {
            // A station joined the hosted network
            this_->usersCounter_->newUserConnected(this_->DOT11_MAC_ADDRESS_to_string(pPeerStateChange->NewState.PeerMacAddress));
        }
        else if (wlan_hosted_network_peer_state_invalid == pPeerStateChange->NewState.PeerAuthState)
        {
            // A station left the hosted network
            this_->usersCounter_->userDiconnected(this_->DOT11_MAC_ADDRESS_to_string(pPeerStateChange->NewState.PeerMacAddress));
        }
    }
    else
    {
        //Q_ASSERT(false);
        qCDebug(LOG_WLAN_MANAGER) << "wlanNotificationCallback, NotificationCode =" << pNotifData->NotificationCode;
    }
}

void WlanManager::safeCloseHandle()
{
    if (!bDllFuncsLoaded_)
    {
        return;
    }

    if (wlanHandle_ != NULL)
    {
        WlanCloseHandle_f(wlanHandle_, NULL);
        wlanHandle_ = NULL;
    }
}

QString WlanManager::makeDebugString_wlan_hosted_network_state_change(PWLAN_NOTIFICATION_DATA pNotifData)
{
    WLAN_HOSTED_NETWORK_STATE_CHANGE *stateChange = (WLAN_HOSTED_NETWORK_STATE_CHANGE *)pNotifData->pData;
    Q_ASSERT(pNotifData->dwDataSize == sizeof(WLAN_HOSTED_NETWORK_STATE_CHANGE));
    QString ret = "wlan_hosted_network_state_change;";
    ret += "OldState=" + WLAN_HOSTED_NETWORK_STATE_to_string(stateChange->OldState);
    ret += ";NewState=" + WLAN_HOSTED_NETWORK_STATE_to_string(stateChange->NewState);
    ret += ";Reason=" + WLAN_HOSTED_NETWORK_REASON_to_string(stateChange->StateChangeReason);
    return ret;
}

QString WlanManager::WLAN_HOSTED_NETWORK_STATE_to_string(int state)
{
    if (state == wlan_hosted_network_unavailable)
        return "wlan_hosted_network_unavailable";
    else if (state == wlan_hosted_network_idle)
        return "wlan_hosted_network_idle";
    else if (state == wlan_hosted_network_active)
        return "wlan_hosted_network_active";
    else
        return "Unknown";
}

QString WlanManager::WLAN_HOSTED_NETWORK_REASON_to_string(int reason)
{
    switch (reason)
    {
        case wlan_hosted_network_reason_success:
            return "wlan_hosted_network_reason_success";
        case wlan_hosted_network_reason_unspecified:
            return "wlan_hosted_network_reason_unspecified";
        case wlan_hosted_network_reason_bad_parameters:
            return "wlan_hosted_network_reason_bad_parameters";
        case wlan_hosted_network_reason_service_shutting_down:
            return "wlan_hosted_network_reason_service_shutting_down";
        case wlan_hosted_network_reason_insufficient_resources:
            return "wlan_hosted_network_reason_insufficient_resources";
        case wlan_hosted_network_reason_elevation_required:
            return "wlan_hosted_network_reason_elevation_required";
        case wlan_hosted_network_reason_read_only:
            return "wlan_hosted_network_reason_read_only";
        case wlan_hosted_network_reason_persistence_failed:
            return "wlan_hosted_network_reason_persistence_failed";
        case wlan_hosted_network_reason_crypt_error:
            return "wlan_hosted_network_reason_crypt_error";
        case wlan_hosted_network_reason_impersonation:
            return "wlan_hosted_network_reason_impersonation";
        case wlan_hosted_network_reason_stop_before_start:
            return "wlan_hosted_network_reason_stop_before_start";
        case wlan_hosted_network_reason_interface_available:
            return "wlan_hosted_network_reason_interface_available";
        case wlan_hosted_network_reason_interface_unavailable:
            return "wlan_hosted_network_reason_interface_unavailable";
        case wlan_hosted_network_reason_miniport_stopped:
            return "wlan_hosted_network_reason_miniport_stopped";
        case wlan_hosted_network_reason_miniport_started:
            return "wlan_hosted_network_reason_miniport_started";
        case wlan_hosted_network_reason_incompatible_connection_started:
            return "wlan_hosted_network_reason_incompatible_connection_started";
        case wlan_hosted_network_reason_incompatible_connection_stopped:
            return "wlan_hosted_network_reason_incompatible_connection_stopped";
        case wlan_hosted_network_reason_user_action:
            return "wlan_hosted_network_reason_user_action";
        case wlan_hosted_network_reason_client_abort:
            return "wlan_hosted_network_reason_client_abort";
        case wlan_hosted_network_reason_ap_start_failed:
            return "wlan_hosted_network_reason_ap_start_failed";
        case wlan_hosted_network_reason_peer_arrived:
            return "wlan_hosted_network_reason_peer_arrived";
        case wlan_hosted_network_reason_peer_departed:
            return "wlan_hosted_network_reason_peer_departed";
        case wlan_hosted_network_reason_peer_timeout:
            return "wlan_hosted_network_reason_peer_timeout";
        case wlan_hosted_network_reason_gp_denied:
            return "wlan_hosted_network_reason_gp_denied";
        case wlan_hosted_network_reason_service_unavailable:
            return "wlan_hosted_network_reason_service_unavailable";
        case wlan_hosted_network_reason_device_change:
            return "wlan_hosted_network_reason_device_change";
        case wlan_hosted_network_reason_properties_change:
            return "wlan_hosted_network_reason_properties_change";
        case wlan_hosted_network_reason_virtual_station_blocking_use:
            return "wlan_hosted_network_reason_virtual_station_blocking_use";
        case wlan_hosted_network_reason_service_available_on_virtual_station:
            return "wlan_hosted_network_reason_service_available_on_virtual_station";
        default:
            return "Unknown";
    }
}

void WlanManager::updateHostedNetworkInterfaceGuid()
{
    QMutexLocker locker(&mutex_);

    if (!bDllFuncsLoaded_)
    {
        return;
    }

    DWORD dwError = ERROR_SUCCESS;
    PWLAN_HOSTED_NETWORK_STATUS pAPStatus = NULL;                        // hosted network status

    dwError = WlanHostedNetworkQueryStatus_f(wlanHandle_, &pAPStatus, NULL);
    if (dwError != ERROR_SUCCESS)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WlanHostedNetworkQueryStatus failed: " << dwError;
        return;
    }

    interfaceGuid_ = pAPStatus->IPDeviceID;
    WlanFreeMemory_f(pAPStatus);
}

QString WlanManager::DOT11_MAC_ADDRESS_to_string(DOT11_MAC_ADDRESS &macAddress)
{
    char buf[128];
    sprintf(buf, "%02x%02x%02x%02x%02x%02x",
            macAddress[0], macAddress[1], macAddress[2],
            macAddress[3], macAddress[4], macAddress[5]);
    return QString::fromStdString(buf);
}

WlanManager::WlanFreeMemoryHelper::WlanFreeMemoryHelper(void *p, WlanManager::WlanFreeMemory_T *wlanFreeMemFunc)
{
    p_ = p;
    wlanFreeMemFunc_ = wlanFreeMemFunc;
}

WlanManager::WlanFreeMemoryHelper::~WlanFreeMemoryHelper()
{
    if (p_ && wlanFreeMemFunc_)
    {
        wlanFreeMemFunc_(p_);
    }
}
