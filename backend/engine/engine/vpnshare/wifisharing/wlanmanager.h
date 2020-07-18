#ifndef WLANMANAGER_H
#define WLANMANAGER_H

#include <QObject>
#include <QMutex>
#include <windows.h>
#include <wlanapi.h>
#include "../connecteduserscounter.h"

class WlanManager : public QObject
{
    Q_OBJECT
public:
    explicit WlanManager(QObject *parent);
    virtual ~WlanManager();

    bool isSupported();
    bool init();
    void deinit();

    bool start(const QString &ssid, const QString &password);
    void getHostedNetworkInterfaceGuid(GUID &interfaceGuid);

    int getConnectedUsersCount();

signals:
    void wlanStarted();
    void usersCountChanged();

private:
    bool bDllFuncsLoaded_;
    HANDLE wlanHandle_;
    DWORD serverVersion_;
    GUID interfaceGuid_;

    ConnectedUsersCounter *usersCounter_;

    QMutex mutex_;

    QString ssid_;
    QString password_;

    static VOID WINAPI wlanNotificationCallback(PWLAN_NOTIFICATION_DATA, PVOID);
    void safeCloseHandle();
    QString makeDebugString_wlan_hosted_network_state_change(PWLAN_NOTIFICATION_DATA pNotifData);
    QString WLAN_HOSTED_NETWORK_STATE_to_string(int state);
    QString WLAN_HOSTED_NETWORK_REASON_to_string(int reason);
    void updateHostedNetworkInterfaceGuid();
    QString DOT11_MAC_ADDRESS_to_string(DOT11_MAC_ADDRESS &macAddress);

    //wlan functions
    typedef DWORD WINAPI
        WlanOpenHandle_T(_In_ DWORD dwClientVersion, _Reserved_ PVOID pReserved, _Out_ PDWORD pdwNegotiatedVersion, _Out_ PHANDLE phClientHandle);

    typedef DWORD WINAPI
        WlanRegisterNotification_T(
            _In_ HANDLE hClientHandle,
            _In_ DWORD dwNotifSource,
            _In_ BOOL bIgnoreDuplicate,
            _In_opt_ WLAN_NOTIFICATION_CALLBACK funcCallback,
            _In_opt_ PVOID pCallbackContext,
            _Reserved_ PVOID pReserved,
            _Out_opt_ PDWORD pdwPrevNotifSource
        );

    typedef DWORD WINAPI
        WlanHostedNetworkInitSettings_T
        (
            _In_        HANDLE                          hClientHandle,
            _Out_opt_   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
            _Reserved_  PVOID                           pvReserved
        );

    typedef DWORD WINAPI
        WlanHostedNetworkQueryStatus_T
        (
            _In_        HANDLE                          hClientHandle,
            _Outptr_    PWLAN_HOSTED_NETWORK_STATUS*    ppWlanHostedNetworkStatus,
            _Reserved_  PVOID                           pvReserved
        );

    typedef VOID WINAPI WlanFreeMemory_T(_In_ PVOID pMemory);

    typedef DWORD WINAPI WlanHostedNetworkQueryProperty_T
        (
            _In_                                HANDLE                      hClientHandle,
            _In_                                WLAN_HOSTED_NETWORK_OPCODE  OpCode,
            _Out_                               PDWORD                      pdwDataSize,
            _Outptr_result_bytebuffer_(*pdwDataSize)    PVOID*                      ppvData,
            _Out_                               PWLAN_OPCODE_VALUE_TYPE     pWlanOpcodeValueType,
            _Reserved_  PVOID                                               pvReserved
        );

    typedef DWORD WINAPI WlanCloseHandle_T(_In_ HANDLE hClientHandle, _Reserved_ PVOID pReserved);

    typedef DWORD WINAPI WlanHostedNetworkForceStop_T(
            _In_        HANDLE                          hClientHandle,
            _Out_opt_   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
            _Reserved_  PVOID                           pvReserved
        );

    typedef DWORD WINAPI WlanHostedNetworkStopUsing_T(
            _In_        HANDLE                          hClientHandle,
            _Out_opt_   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
            _Reserved_  PVOID                           pvReserved
        );

    typedef DWORD WINAPI WlanHostedNetworkSetProperty_T(
            _In_                        HANDLE                          hClientHandle,
            _In_                        WLAN_HOSTED_NETWORK_OPCODE      OpCode,
            _In_                        DWORD                           dwDataSize,
            _In_reads_bytes_(dwDataSize)     PVOID                      pvData,
            _Out_opt_                   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
            _Reserved_                  PVOID                           pvReserved
        );

    typedef DWORD WINAPI WlanHostedNetworkSetSecondaryKey_T (
            _In_        HANDLE                          hClientHandle,
            _In_        DWORD                           dwKeyLength,
            _In_reads_bytes_(dwKeyLength) PUCHAR        pucKeyData,
            _In_        BOOL                            bIsPassPhrase,
            _In_        BOOL                            bPersistent,
            _Out_opt_   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
            _Reserved_  PVOID                           pvReserved
        );

   typedef DWORD WINAPI WlanHostedNetworkStartUsing_T (
        _In_        HANDLE                          hClientHandle,
        _Out_opt_   PWLAN_HOSTED_NETWORK_REASON     pFailReason,
        _Reserved_  PVOID                           pvReserved
    );


    WlanOpenHandle_T *WlanOpenHandle_f;
    WlanRegisterNotification_T *WlanRegisterNotification_f;
    WlanHostedNetworkInitSettings_T *WlanHostedNetworkInitSettings_f;
    WlanHostedNetworkQueryStatus_T *WlanHostedNetworkQueryStatus_f;
    WlanFreeMemory_T *WlanFreeMemory_f;
    WlanHostedNetworkQueryProperty_T *WlanHostedNetworkQueryProperty_f;
    WlanCloseHandle_T *WlanCloseHandle_f;
    WlanHostedNetworkForceStop_T *WlanHostedNetworkForceStop_f;
    WlanHostedNetworkStopUsing_T *WlanHostedNetworkStopUsing_f;
    WlanHostedNetworkSetProperty_T *WlanHostedNetworkSetProperty_f;
    WlanHostedNetworkSetSecondaryKey_T *WlanHostedNetworkSetSecondaryKey_f;
    WlanHostedNetworkStartUsing_T *WlanHostedNetworkStartUsing_f;

    class WlanFreeMemoryHelper
    {
    public:
        WlanFreeMemoryHelper(void *p, WlanFreeMemory_T *wlanFreeMemFunc);
        ~WlanFreeMemoryHelper();
    private:
        void *p_;
        WlanFreeMemory_T *wlanFreeMemFunc_;
    };
};

#endif // WLANMANAGER_H
