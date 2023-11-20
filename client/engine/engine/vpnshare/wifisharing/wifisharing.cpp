#include "wifisharing.h"
#include <QTimer>
#include <QDebug>
#include <winsock2.h>
#include <iphlpapi.h>
#include <QElapsedTimer>
#include "utils/logger.h"
#include "utils/winutils.h"
#include "wifidirectmanager.h"
#include "icsmanager.h"

GUID getGuidForPrimaryAdapter(bool &bSuccess, bool bForWindscribeAdapter, const QString &vpnAdapterName);

WifiSharing::WifiSharing(QObject *parent, IHelper *helper) : QObject(parent),
    isSharingStarted_(false), sharingState_(STATE_DISCONNECTED)
{
    icsManager_ = new IcsManager(this, helper);
    wifiDirectManager_ = new WiFiDirectManager(this);
    connect(wifiDirectManager_, SIGNAL(started()), SLOT(onWifiDirectStarted()));
    connect(wifiDirectManager_, SIGNAL(usersCountChanged()), SIGNAL(usersCountChanged()));
}

WifiSharing::~WifiSharing()
{
    //stopSharing();
    delete icsManager_;
}

bool WifiSharing::isSupported()
{
    return icsManager_->isSupportedIcs() && wifiDirectManager_->isSupported();
}

void WifiSharing::startSharing(const QString &ssid, const QString &password)
{
    if (!icsManager_->startIcs())
    {
        qCDebug(LOG_WIFI_SHARED) << "icsManager_->startIcs() failed";
        return;
    }

    if (!wifiDirectManager_->start(ssid, password))
    {
        qCDebug(LOG_WIFI_SHARED) << "wlanManager_->start() failed";
        return;
    }
    ssid_ = ssid;
    isSharingStarted_ = true;
}

void WifiSharing::stopSharing()
{
    icsManager_->stopIcs();
    qCDebug(LOG_WIFI_SHARED) << "WifiSharing::stopSharing() stopIcs";
    wifiDirectManager_->stop();
    qCDebug(LOG_WIFI_SHARED) << "WifiSharing::stopSharing() deinit";
    isSharingStarted_ = false;
}

bool WifiSharing::isSharingStarted()
{
    return isSharingStarted_;
}

void WifiSharing::switchSharingForDisconnected()
{
    vpnAdapterName_.clear();
    sharingState_ = STATE_DISCONNECTED;
}

void WifiSharing::switchSharingForConnected(const QString &vpnAdapterName)
{
    vpnAdapterName_ = vpnAdapterName;
    sharingState_ = STATE_CONNECTING_CONNECTED;
    if (isSharingStarted_)
    {
        updateICS(vpnAdapterName);
    }
}

int WifiSharing::getConnectedUsersCount()
{
    return wifiDirectManager_->getConnectedUsersCount();
}

void WifiSharing::onWifiDirectStarted()
{
    if (isSharingStarted_ && !vpnAdapterName_.isEmpty())
    {
        qCDebug(LOG_WIFI_SHARED) << "WifiSharing::onWlanStarted";
        updateICS(vpnAdapterName_);
    }
}

void WifiSharing::updateICS(const QString &vpnAdapterName)
{
    qCDebug(LOG_WIFI_SHARED) << "WifiSharing::updateICS()";
    icsManager_->changeIcs(vpnAdapterName);
    //FIXME:
//    qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::updateICS()";
//    bool bSuccess;
//    GUID guidAdapter = getGuidForPrimaryAdapter(bSuccess, state == STATE_CONNECTING_CONNECTED, vpnAdapterName);

//    if (!bSuccess)
//    {
//        qCDebug(LOG_WLAN_MANAGER) << "Can't detect primary network adapter for sharing";
//    }
//    else
//    {
//        GUID wlanGuid;
//        tetheringManager_->getHostedNetworkInterfaceGuid(wlanGuid);
//        icsManager_->changeIcsSettings(guidAdapter, wlanGuid);
//    }
}

GUID getGuidForPrimaryAdapter(bool &bSuccess, bool bForWindscribeAdapter, const QString &vpnAdapterName)
{
    DWORD dwBestIfIndex;
    if (GetBestInterface(INADDR_ANY, &dwBestIfIndex) != NO_ERROR)
    {
        bSuccess = false;
        return GUID();
    }

    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG iterations = 0;
    const int MAX_TRIES = 5;
    QByteArray arr;

    outBufLen  = arr.size();
    do {
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, NULL, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            arr.resize(outBufLen);
        }
        else
        {
            break;
        }
        iterations++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));

    if (dwRetVal != NO_ERROR)
    {
        bSuccess = false;
        return GUID();
    }

    // first check Windscribe VPN tap adapter if it active, then return it
    PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)arr.data();
    if (bForWindscribeAdapter)
    {
        while (pCurrAddresses)
        {
            if (wcsstr(pCurrAddresses->Description, vpnAdapterName.toStdWString().c_str()) != 0)
            //if (pCurrAddresses->OperStatus == IfOperStatusUp)
            {
                    qCDebug(LOG_WIFI_SHARED) << "Detected Windscribe VPN tap adapter" << QString::fromStdString(pCurrAddresses->AdapterName) << QString::fromStdWString(pCurrAddresses->FriendlyName);
                    bSuccess = true;
                    GUID guid = WinUtils::stringToGuid(pCurrAddresses->AdapterName);
                    return guid;
            }

            pCurrAddresses = pCurrAddresses->Next;
        }
    }

    // if Windscribe VPN not active, select primary best network adapter
    pCurrAddresses = (PIP_ADAPTER_ADDRESSES)arr.data();
    while (pCurrAddresses)
    {
        if (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD || pCurrAddresses->IfType == IF_TYPE_IEEE80211)
        if (pCurrAddresses->IfIndex == dwBestIfIndex)
        {
            qCDebug(LOG_WIFI_SHARED) << "Detected primary network adapter" << QString::fromStdString(pCurrAddresses->AdapterName) << QString::fromStdWString(pCurrAddresses->FriendlyName);
            bSuccess = true;
            GUID guid = WinUtils::stringToGuid(pCurrAddresses->AdapterName);
            return guid;
        }

        pCurrAddresses = pCurrAddresses->Next;
    }

    bSuccess = false;
    return GUID();
}


