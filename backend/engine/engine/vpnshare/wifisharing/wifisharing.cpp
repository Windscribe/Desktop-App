#include "wifisharing.h"
#include <QTimer>
#include <QDebug>
#include <winsock2.h>
#include <iphlpapi.h>
#include <QElapsedTimer>
#include "utils/logger.h"
#include "wlanmanager.h"
#include "icsmanager.h"

GUID getGuidForPrimaryAdapter(bool &bSuccess, bool bForWindscribeAdapter, const QString &vpnAdapterName);
GUID stringToGuid(PCHAR str);

WifiSharing::WifiSharing(QObject *parent, IHelper *helper) : QObject(parent),
    isSharingStarted_(false), sharingState_(STATE_DISCONNECTED)
{
    icsManager_ = new IcsManager(this, helper);
    wlanManager_ = new WlanManager(this);
    connect(wlanManager_, SIGNAL(wlanStarted()), SLOT(onWlanStarted()));
    connect(wlanManager_, SIGNAL(usersCountChanged()), SIGNAL(usersCountChanged()));
    connect(&timerForUpdateIcsDisconnected_, SIGNAL(timeout()), SLOT(onTimerForUpdateIcsDisconnected()));
    timerForUpdateIcsDisconnected_.setSingleShot(true);
}

WifiSharing::~WifiSharing()
{
    timerForUpdateIcsDisconnected_.stop();
    //stopSharing();
    delete icsManager_;
}

bool WifiSharing::isSupported()
{
    return icsManager_->isSupportedIcs() && wlanManager_->isSupported();
}

void WifiSharing::startSharing(const QString &ssid, const QString &password)
{
    if (!wlanManager_->init())
    {
        qCDebug(LOG_WLAN_MANAGER) << "wlanManager_->init() failed";
        return;
    }

    if (!icsManager_->startIcs())
    {
        qCDebug(LOG_WLAN_MANAGER) << "icsManager_->startIcs() failed";
        return;
    }

    if (!wlanManager_->start(ssid, password))
    {
        qCDebug(LOG_WLAN_MANAGER) << "wlanManager_->start() failed";
        return;
    }
    ssid_ = ssid;
    isSharingStarted_ = true;
}

void WifiSharing::stopSharing()
{
    icsManager_->stopIcs();
    qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::stopSharing() stopIcs";
    wlanManager_->deinit();
    qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::stopSharing() deinit";
    isSharingStarted_ = false;
}

bool WifiSharing::isSharingStarted()
{
    return isSharingStarted_;
}

void WifiSharing::switchSharingForDisconnected()
{
    if (timerForUpdateIcsDisconnected_.isActive())
    {
        timerForUpdateIcsDisconnected_.stop();
    }
    sharingState_ = STATE_DISCONNECTED;
    timerForUpdateIcsDisconnected_.start(DELAY_FOR_UPDATE_ICS_DISCONNECTED);
}

void WifiSharing::switchSharingForConnectingOrConnected(const QString &vpnAdapterName)
{
    vpnAdapterName_ = vpnAdapterName;
    if (timerForUpdateIcsDisconnected_.isActive())
    {
        timerForUpdateIcsDisconnected_.stop();
    }
    sharingState_ = STATE_CONNECTING_CONNECTED;
    if (isSharingStarted_)
    {
        updateICS(sharingState_, vpnAdapterName);
    }
}

int WifiSharing::getConnectedUsersCount()
{
    return wlanManager_->getConnectedUsersCount();
}

bool WifiSharing::isUpdateIcsInProgress()
{
    return icsManager_->isUpdateIcsInProgress();
}

void WifiSharing::onWlanStarted()
{
    if (isSharingStarted_)
    {
        qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::onWlanStarted";
        updateICS(sharingState_, vpnAdapterName_);
    }
}

void WifiSharing::onTimerForUpdateIcsDisconnected()
{
    qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::onTimerForUpdateIcsDisconnected";
    timerForUpdateIcsDisconnected_.stop();
    if (isSharingStarted_)
    {
        // switch sharing for disconnected state make with delay
        updateICS(sharingState_, vpnAdapterName_);
    }
}

void WifiSharing::updateICS(int state, const QString &vpnAdapterName)
{
    qCDebug(LOG_WLAN_MANAGER) << "WifiSharing::updateICS()";
    bool bSuccess;
    GUID guidAdapter = getGuidForPrimaryAdapter(bSuccess, state == STATE_CONNECTING_CONNECTED, vpnAdapterName);

    if (!bSuccess)
    {
        qCDebug(LOG_WLAN_MANAGER) << "Can't detect primary network adapter for sharing";
    }
    else
    {
        GUID wlanGuid;
        wlanManager_->getHostedNetworkInterfaceGuid(wlanGuid);
        icsManager_->changeIcsSettings(guidAdapter, wlanGuid);
    }
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
                    qCDebug(LOG_WLAN_MANAGER) << "Detected Windscribe VPN tap adapter" << QString::fromStdString(pCurrAddresses->AdapterName) << QString::fromStdWString(pCurrAddresses->FriendlyName);
                    bSuccess = true;
                    GUID guid = stringToGuid(pCurrAddresses->AdapterName);
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
            qCDebug(LOG_WLAN_MANAGER) << "Detected primary network adapter" << QString::fromStdString(pCurrAddresses->AdapterName) << QString::fromStdWString(pCurrAddresses->FriendlyName);
            bSuccess = true;
            GUID guid = stringToGuid(pCurrAddresses->AdapterName);
            return guid;
        }

        pCurrAddresses = pCurrAddresses->Next;
    }

    bSuccess = false;
    return GUID();
}

GUID stringToGuid(PCHAR str)
{
    GUID guid;
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    sscanf_s(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);

    guid.Data1 = p0;
    guid.Data2 = p1;
    guid.Data3 = p2;
    guid.Data4[0] = p3;
    guid.Data4[1] = p4;
    guid.Data4[2] = p5;
    guid.Data4[3] = p6;
    guid.Data4[4] = p7;
    guid.Data4[5] = p8;
    guid.Data4[6] = p9;
    guid.Data4[7] = p10;

    return guid;
}


