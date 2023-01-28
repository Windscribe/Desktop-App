#include "ikev2connection_win.h"
#include "utils/logger.h"
#include <windows.h>
#include <windns.h>
#include <QCoreApplication>
#include "utils/hardcodedsettings.h"
#include "engine/taputils/checkadapterenable.h"
#include "utils/winutils.h"
#include "utils/ras_service_win.h"
#include "adapterutils_win.h"

#define IKEV2_CONNECTION_NAME  L"Windscribe IKEv2"

IKEv2Connection_win *IKEv2Connection_win::this_ = NULL;
// static global variable, because we reinstall WAN only once during the lifetime of the process
bool IKEv2Connection_win::wanReinstalled_ = false;


IKEv2Connection_win::IKEv2Connection_win(QObject *parent, IHelper *helper) : IConnection(parent),
    state_(STATE_DISCONNECTED), initialEnableIkev2Compression_(false),
    isAutomaticConnectionMode_(false), connHandle_(NULL), mutex_(QRecursiveMutex()),
    disconnectLogic_(this), cntFailedConnectionAttempts_(0)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_);

    Q_ASSERT(this_ == NULL);
    this_ = this;
    initMapConnStates();
    timerControlConnection_.setInterval(CONTROL_TIMER_PERIOD);
    connect(&timerControlConnection_, &QTimer::timeout, this, &IKEv2Connection_win::onTimerControlConnection);
    connect(&disconnectLogic_, &IKEv2ConnectionDisconnectLogic_win::disconnected, this, &IKEv2Connection_win::onHandleDisconnectLogic);
    Q_ASSERT(state_ == STATE_DISCONNECTED);
}

IKEv2Connection_win::~IKEv2Connection_win()
{
    this_ = NULL;
}

void IKEv2Connection_win::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password, const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    Q_UNUSED(dnsHostName);
    Q_UNUSED(proxySettings);
    Q_UNUSED(wireGuardConfig);

    QMutexLocker locker(&mutex_);

    Q_ASSERT(state_ == STATE_DISCONNECTED);

    initialUrl_ = configPathOrUrl;
    initialIp_ = ip;
    initialUsername_ = username;
    initialPassword_ = password;
    initialEnableIkev2Compression_ = isEnableIkev2Compression;
    isAutomaticConnectionMode_ = isAutomaticConnectionMode;

    cntFailedConnectionAttempts_ = 0;

    doConnect();
}

void IKEv2Connection_win::startDisconnect()
{
    QMutexLocker locker(&mutex_);

    // if already in disconnecting state, then skip dispatch
    if (state_ == STATE_DISCONNECTING)
    {
        return;
    }

    if (connHandle_)
    {
        state_ = STATE_DISCONNECTING;
        disconnectLogic_.startDisconnect(connHandle_);
    }
    else
    {
        timerControlConnection_.stop();
        state_ = STATE_DISCONNECTED;
        emit disconnected();
    }
}

bool IKEv2Connection_win::isDisconnected() const
{
    QMutexLocker locker(&mutex_);
    return  state_ == STATE_DISCONNECTED;
}

/*QString IKEv2Connection_win::getConnectedTapTunAdapterName()
{
    return QString::fromStdWString(IKEV2_CONNECTION_NAME);
}*/

void IKEv2Connection_win::removeIkev2ConnectionFromOS()
{
    DWORD dwErr = RasDeleteEntry(NULL, IKEV2_CONNECTION_NAME);
    if (dwErr != ERROR_SUCCESS && dwErr != ERROR_CANNOT_FIND_PHONEBOOK_ENTRY)
    {
        qCDebug(LOG_IKEV2) << "RasDeleteEntry failed with error:" << dwErr;
    }
}

QVector<HRASCONN> IKEv2Connection_win::getActiveWindscribeConnections()
{
    QVector<HRASCONN> v;


    DWORD dwCb = 0;
    DWORD dwConnections = 0;
    LPRASCONN lpRasConn = NULL;

    DWORD dwRet = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);
    if (dwRet == ERROR_BUFFER_TOO_SMALL)
    {
        QByteArray arr(dwCb, Qt::Uninitialized);
        lpRasConn = (LPRASCONN)arr.data();
        lpRasConn[0].dwSize = sizeof(RASCONN);
        dwRet = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);
        if (dwRet == ERROR_SUCCESS)
        {
            for (DWORD i = 0; i < dwConnections; i++)
            {
                if (wcscmp(lpRasConn[i].szEntryName, IKEV2_CONNECTION_NAME) == 0)
                {
                    v << lpRasConn[i].hrasconn;
                }
            }

        }
    }

    return v;
}

void IKEv2Connection_win::continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
}

void IKEv2Connection_win::continueWithPassword(const QString & /*password*/)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
}

void IKEv2Connection_win::onTimerControlConnection()
{
    QMutexLocker locker(&mutex_);

    if (connHandle_)
    {
        if (state_ == STATE_DISCONNECTING)
        {
            // waiting signal to onHandleDisconnectLogic() from disconnectLogic object
            return;
        }
        else
        {
            RASCONNSTATUS status;
            memset(&status, 0, sizeof(status));
            status.dwSize = sizeof(status);
            DWORD err = RasGetConnectStatus(connHandle_, &status);
            if (err == ERROR_INVALID_HANDLE || err == ERROR_NO_CONNECTION)
            {
                connHandle_ = NULL;
                timerControlConnection_.stop();
                helper_->disableDnsLeaksProtection();
                helper_->removeHosts();
                state_ = STATE_DISCONNECTED;
                emit disconnected();
            }
            else
            {
                RAS_STATS stats;
                stats.dwSize = sizeof(RAS_STATS);
                if (RasGetLinkStatistics(connHandle_, 1, &stats) == ERROR_SUCCESS)
                {
                    emit statisticsUpdated(stats.dwBytesRcved, stats.dwBytesXmited, false);
                    RasClearLinkStatistics(connHandle_, 1);
                }
            }
        }
    }
    else
    {
        Q_ASSERT(false);
    }
}

void IKEv2Connection_win::handleAuthError()
{
    QMutexLocker locker(&mutex_);
    // auth error
    doBlockingDisconnect();
    connHandle_ = NULL;
    emit error(ProtoTypes::ConnectError::AUTH_ERROR);
    helper_->disableDnsLeaksProtection();
    helper_->removeHosts();
    state_ = STATE_DISCONNECTED;
    emit disconnected();
}

void IKEv2Connection_win::handleErrorReinstallWan()
{
    QMutexLocker locker(&mutex_);

    // skip dispatch if we disconnecting
    if (state_ == STATE_DISCONNECTING)
    {
        return;
    }

    doBlockingDisconnect();

    connHandle_ = NULL;
    helper_->disableDnsLeaksProtection();
    helper_->removeHosts();

    // With issues 498 and 576, it was found that only restarting the app would rectify the
    // AuthNotify error.  When exiting the app, we run this method, so hopefully running
    // it here helps with the aforementioned issues, which are not reproducible, and does
    // no harm elsewhere.
    removeIkev2ConnectionFromOS();

    if (isAutomaticConnectionMode_)
    {
        cntFailedConnectionAttempts_++;

        if (cntFailedConnectionAttempts_ >= (MAX_FAILED_CONNECTION_ATTEMPTS_FOR_AUTOMATIC_MODE))
        {
            state_ = STATE_DISCONNECTED;
            emit error(ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT);
        }
        else
        {
            if (WinUtils::isWindows10orGreater())
            {

                if (CheckAdapterEnable::isAdapterDisabled(helper_, "WAN Miniport (IKEv2)") || CheckAdapterEnable::isAdapterDisabled(helper_, "WAN Miniport (IP)"))
                {
                    qCDebug(LOG_IKEV2) << "WAN Miniport (IKEv2) or WAN Miniport (IP) disabled, try enable it";
                    helper_->enableWanIkev2();
                    // pause 3 secs, before connect
                    QThread::msleep(3000);
                    doConnect();
                }
                else
                {
                    if (!wanReinstalled_)
                    {
                        helper_->reinstallWanIkev2();
                        wanReinstalled_ = true;
                        qCDebug(LOG_IKEV2) << "Reinstalled Wan IKEv2";
                        // pause 3 secs, before connect
                        QThread::msleep(3000);
                        doConnect();
                    }
                    else
                    {
                        state_ = STATE_DISCONNECTED;
                        emit error(ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT);
                    }
                }
            }
            else
            {
                doConnect();
            }
        }
    }
    else
    {
        cntFailedConnectionAttempts_++;

        if (cntFailedConnectionAttempts_ == (MAX_FAILED_CONNECTION_ATTEMPTS - 1))
        {
            if (WinUtils::isWindows10orGreater() && !wanReinstalled_)
            {
                helper_->reinstallWanIkev2();
                wanReinstalled_ = true;
                qCDebug(LOG_IKEV2) << "Reinstalled Wan IKEv2";
                // pause 3 secs, before connect
                QThread::msleep(3000);
                doConnect();
            }
            else
            {
                state_ = STATE_DISCONNECTED;
                emit error(ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT);
            }
        }
        else if (cntFailedConnectionAttempts_ >= MAX_FAILED_CONNECTION_ATTEMPTS)
        {
            state_ = STATE_DISCONNECTED;
            emit error(ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT);
        }
        else
        {
            if (WinUtils::isWindows10orGreater())
            {
                if (CheckAdapterEnable::isAdapterDisabled(helper_, "WAN Miniport (IKEv2)") || CheckAdapterEnable::isAdapterDisabled(helper_, "WAN Miniport (IP)"))
                {
                    qCDebug(LOG_IKEV2) << "WAN Miniport (IKEv2) or WAN Miniport (IP) disabled, try enable it";
                    helper_->enableWanIkev2();
                    // pause 3 secs, before connect
                    QThread::msleep(3000);
                }
            }
            doConnect();
        }
    }
}

void IKEv2Connection_win::onHandleDisconnectLogic()
{
    connHandle_ = NULL;
    helper_->disableDnsLeaksProtection();
    helper_->removeHosts();
    timerControlConnection_.stop();
    state_ = STATE_DISCONNECTED;
    emit disconnected();
}

void IKEv2Connection_win::doConnect()
{
    bool ikev2DeviceInitialized = false;
    RASDEVINFO devInfo;
    if (!getIKEv2Device(&devInfo))
    {
        qCDebug(LOG_IKEV2) << "Trying resrart SstpSvc and RasMan";
        if (!RAS_Service_win::instance().restartRASServices(helper_))
        {
            qCDebug(LOG_IKEV2) << "Failed to start SstpSvc and/or RasMan services, so return disconnect error";
        }
        else
        {
            qCDebug(LOG_IKEV2) << "SstpSvc and/or RasMan services restarted, so try getIKEv2Device again";
            ikev2DeviceInitialized = getIKEv2Device(&devInfo);
            if (!ikev2DeviceInitialized)
            {
                qCDebug(LOG_IKEV2) << "getIKEv2Device failed again";
            }
        }
    }
    else
    {
        ikev2DeviceInitialized = true;
    }

    if (!ikev2DeviceInitialized)
    {
        state_ = STATE_DISCONNECTED;
        emit error(ProtoTypes::ConnectError::IKEV_NOT_FOUND_WIN);
        return;
    }


    RASENTRY rasEntry;
    memset(&rasEntry, 0, sizeof(rasEntry));
    rasEntry.dwSize = offsetof(RASENTRY, ipv6addr);
    //rasEntry.dwSize = sizeof(RASENTRY);

    wcscpy_s(rasEntry.szLocalPhoneNumber, initialUrl_.toStdWString().c_str());
    wcscpy_s(rasEntry.szDeviceName, devInfo.szDeviceName);
    wcscpy_s(rasEntry.szDeviceType, devInfo.szDeviceType);

    rasEntry.dwfOptions = RASEO_RequireEAP  /*| RASEO_RemoteDefaultGateway*/;
    if (initialEnableIkev2Compression_)
    {
        rasEntry.dwfOptions = rasEntry.dwfOptions | RASEO_IpHeaderCompression | RASEO_SwCompression;
    }

    rasEntry.dwfOptions2 = RASEO2_DontNegotiateMultilink | RASEO2_ReconnectIfDropped /*| RASEO2_IPv6RemoteDefaultGateway*/ | RASEO2_IPv4ExplicitMetric | RASEO2_IPv6ExplicitMetric;
    rasEntry.dwFramingProtocol = RASFP_Ppp;
    rasEntry.dwEncryptionType = ET_RequireMax;
    rasEntry.dwType = RASET_Vpn;
    rasEntry.dwVpnStrategy = VS_Ikev2Only;
    rasEntry.dwfNetProtocols = RASNP_Ip | RASNP_Ipv6;
    rasEntry.dwRedialCount = 3;
    rasEntry.dwRedialPause = 60;
    rasEntry.dwIPv4InterfaceMetric = 1;  // minimum ipv4 metric
    rasEntry.dwIPv6InterfaceMetric = 1;  // minimum ipv6 metric
    rasEntry.dwIdleDisconnectSeconds = 60;  // 60 sec disconnect timeout
    //rasEntry.dwNetworkOutageTime = 60; // it's unclear whether this works or not.


    DWORD dwErr = RasSetEntryProperties(NULL, IKEV2_CONNECTION_NAME, &rasEntry, rasEntry.dwSize, NULL, NULL);
    if (dwErr != ERROR_SUCCESS)
    {
        if (dwErr == ERROR_INVALID_SIZE)
        {
            // try manual size (bug in 17133.1, https://answers.microsoft.com/en-us/insider/forum/insider_wintp-insider_update-insiderplat_pc/ras-error-632-on-windows-insider-build-17083/aacf68b9-3171-4342-ab9e-cbd4e278d4a7?page=2)
            rasEntry.dwSize = 6720;
            dwErr = RasSetEntryProperties(NULL, IKEV2_CONNECTION_NAME, &rasEntry, rasEntry.dwSize, NULL, NULL);
        }
        if (dwErr != ERROR_SUCCESS)
        {
            qCDebug(LOG_IKEV2) << "RasSetEntryProperties failed with error:" << dwErr;
            state_ = STATE_DISCONNECTED;
            emit error(ProtoTypes::ConnectError::IKEV_FAILED_SET_ENTRY_WIN);
            return;
        }
    }

    connHandle_ = NULL;

    RASDIALPARAMS dialparams;
    memset(&dialparams, 0, sizeof(dialparams));
    dialparams.dwSize = sizeof(dialparams);

    wcscpy_s(dialparams.szEntryName, IKEV2_CONNECTION_NAME);
    wcscpy_s(dialparams.szUserName, initialUsername_.toStdWString().c_str());
    wcscpy_s(dialparams.szPassword, initialPassword_.toStdWString().c_str());

    dwErr = RasSetEntryDialParams(NULL, &dialparams, FALSE);
    if (dwErr != ERROR_SUCCESS)
    {
        qCDebug(LOG_IKEV2) << "RasSetEntryDialParams failed with error:" << dwErr;
        state_ = STATE_DISCONNECTED;
        emit error(ProtoTypes::ConnectError::IKEV_FAILED_SET_ENTRY_WIN);
        return;
    }

    if (!helper_->addHosts(initialIp_ + " " + initialUrl_))
    {
        qCDebug(LOG_IKEV2) << "Can't modify hosts file";
        state_ = STATE_DISCONNECTED;
        emit error(ProtoTypes::ConnectError::IKEV_FAILED_MODIFY_HOSTS_WIN);
        return;
    }

    helper_->setIKEv2IPSecParameters();
    helper_->enableDnsLeaksProtection();

    // Connecting
    state_ = STATE_CONNECTING;

    dwErr = RasDial(NULL, NULL, &dialparams, 1, (void *)staticRasDialFunc, &connHandle_);
    if (dwErr != ERROR_SUCCESS)
    {
        qCDebug(LOG_IKEV2) << "RasDial failed with error:" << dwErr;
        connHandle_ = NULL;
        helper_->disableDnsLeaksProtection();
        helper_->removeHosts();
        state_ = STATE_DISCONNECTED;
        emit error(ProtoTypes::ConnectError::IKEV_FAILED_SET_ENTRY_WIN);
        return;
    }
}

void IKEv2Connection_win::rasDialFuncCallback(HRASCONN hrasconn, UINT unMsg, tagRASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError)
{
    Q_UNUSED(hrasconn);
    Q_UNUSED(unMsg);
    Q_UNUSED(dwExtendedError);

    QMutexLocker locker(&mutex_);

    // log state
    QString str = rasConnStateToString(rascs);
    if (dwError == 0)
    {
        qCDebug(LOG_IKEV2) << "RasDial state:" << str << "ok" << "(" << state_ << ")";
    }
    else
    {
        wchar_t strErr[1024];
        DWORD result = ::RasGetErrorString(dwError, strErr, 1024);
        if (result != ERROR_SUCCESS)
        {
            // RasGetErrorString will fail if provided a non-RAS error code (e.g. ERROR_IPSEC_IKE_AUTH_FAIL, ERROR_IPSEC_IKE_POLICY_CHANGE)
            WinUtils::Win32GetErrorString(dwError, strErr, _countof(strErr));
        }

        qCDebug(LOG_IKEV2) << "RasDial state:" << str << "Error code:" << dwError << QString::fromWCharArray(strErr) << "(" << state_ << ")";
    }

    // skip dispatch in disconnecting state
    if (state_ == STATE_DISCONNECTING || state_ == STATE_REINSTALL_WAN)
    {
        return;
    }

    if (dwError == 0)
    {
        if (rascs == RASCS_Connected)
        {
            helper_->addIKEv2DefaultRoute();

            timerControlConnection_.setInterval(CONTROL_TIMER_PERIOD);
            QTimer::singleShot(0, &timerControlConnection_, SLOT(start()));
            state_ = STATE_CONNECTED;

            AdapterGatewayInfo cai = AdapterUtils_win::getWindscribeConnectedAdapterInfo();
            emit connected(cai);
        }
    }
    else
    {
        if (state_ == STATE_CONNECTING)
        {
            if (dwError != ERROR_AUTHENTICATION_FAILURE)
            {
                state_ = STATE_REINSTALL_WAN;
                QTimer::singleShot(0, this, SLOT(handleErrorReinstallWan()));
            }
        }

        if (rascs == RASCS_AuthNotify)
        {
            if (dwError == ERROR_AUTHENTICATION_FAILURE)   // Error 691
            {
                state_ = STATE_DISCONNECTING;
                QTimer::singleShot(0, this, SLOT(handleAuthError()));
            }
        }
    }
}

void IKEv2Connection_win::doBlockingDisconnect()
{
    disconnectLogic_.blockSignals(true);
    disconnectLogic_.startDisconnect(connHandle_);
    while (!disconnectLogic_.isDisconnected())
    {
        QThread::msleep(1);
        qApp->processEvents();
    }
    disconnectLogic_.blockSignals(false);
}

bool IKEv2Connection_win::getIKEv2Device(tagRASDEVINFOW *outDevInfo)
{
    RASDEVINFO *devInfo;
    DWORD dwSize = 0;
    DWORD dwNumberDevices = 0;

    DWORD dwErr = RasEnumDevices(0, &dwSize, &dwNumberDevices);
    if (dwErr != ERROR_BUFFER_TOO_SMALL && dwErr != ERROR_SUCCESS)
    {
        qCDebug(LOG_IKEV2) << "RasEnumDevices failed with error:" << dwErr;
        return false;
    }

    QByteArray arr;
    arr.resize(dwSize);
    devInfo = (RASDEVINFO*)arr.data();
    devInfo[0].dwSize = sizeof(RASDEVINFO);

    dwErr = RasEnumDevices(devInfo, &dwSize, &dwNumberDevices);
    if (dwErr == ERROR_SUCCESS)
    {
        QString strLog;

        for (DWORD i = 0; i < dwNumberDevices; ++i)
        {
            QString devtype = QString::fromUtf16((const ushort *)devInfo[i].szDeviceType);
            QString devname = QString::fromUtf16((const ushort *)devInfo[i].szDeviceName);

            devtype = devtype.toLower();
            devname = devname.toLower();

            strLog += devtype + ", " + devname + "; ";

            if (devtype.contains("vpn") && devname.contains("ikev2"))
            {
                if (outDevInfo)
                {
                    *outDevInfo = devInfo[i];
                }
                return true;
            }
        }

        qCDebug(LOG_IKEV2) << "Not found ikev2 device in list:" << strLog;
    }
    else
    {
        qCDebug(LOG_IKEV2) << "RasEnumDevices failed with error:" << dwErr;
        return false;
    }

    return false;
}

void IKEv2Connection_win::initMapConnStates()
{
    mapConnStates_[RASCS_OpenPort] = "OpenPort";
    mapConnStates_[RASCS_PortOpened] = "PortOpened";
    mapConnStates_[RASCS_ConnectDevice] = "ConnectDevice";
    mapConnStates_[RASCS_DeviceConnected] = "DeviceConnected";
    mapConnStates_[RASCS_AllDevicesConnected] = "AllDevicesConnected";
    mapConnStates_[RASCS_Authenticate] = "Authenticate";
    mapConnStates_[RASCS_AuthNotify] = "AuthNotify";
    mapConnStates_[RASCS_AuthRetry] = "AuthRetry";
    mapConnStates_[RASCS_AuthCallback] = "AuthCallback";
    mapConnStates_[RASCS_AuthChangePassword] = "AuthChangePassword";
    mapConnStates_[RASCS_AuthProject] = "AuthProject";
    mapConnStates_[RASCS_AuthLinkSpeed] = "AuthLinkSpeed";
    mapConnStates_[RASCS_AuthAck] = "AuthAck";
    mapConnStates_[RASCS_ReAuthenticate] = "ReAuthenticate";
    mapConnStates_[RASCS_Authenticated] = "Authenticated";
    mapConnStates_[RASCS_PrepareForCallback] = "PrepareForCallback";
    mapConnStates_[RASCS_WaitForModemReset] = "WaitForModemReset";
    mapConnStates_[RASCS_WaitForCallback] = "WaitForCallback";
    mapConnStates_[RASCS_Projected] = "Projected";
    mapConnStates_[RASCS_StartAuthentication] = "StartAuthentication";
    mapConnStates_[RASCS_CallbackComplete] = "CallbackComplete";
    mapConnStates_[RASCS_LogonNetwork] = "LogonNetwork";
    mapConnStates_[RASCS_SubEntryConnected] = "SubEntryConnected";
    mapConnStates_[RASCS_SubEntryDisconnected] = "SubEntryDisconnected";
    mapConnStates_[RASCS_ApplySettings] = "ApplySettings";
    mapConnStates_[RASCS_Interactive] = "Interactive";
    mapConnStates_[RASCS_RetryAuthentication] = "RetryAuthentication";
    mapConnStates_[RASCS_CallbackSetByCaller] = "CallbackSetByCaller";
    mapConnStates_[RASCS_PasswordExpired] = "PasswordExpired";
    mapConnStates_[RASCS_InvokeEapUI] = "InvokeEapUI";
    mapConnStates_[RASCS_Connected] = "Connected";
    mapConnStates_[RASCS_Disconnected] = "Disconnected";
}

QString IKEv2Connection_win::rasConnStateToString(tagRASCONNSTATE state)
{
    auto it = mapConnStates_.find(state);
    if (it != mapConnStates_.end())
    {
        return it.value();
    }
    else
    {
        Q_ASSERT(false);
        return "rasConnStateToString: unknown value";
    }
}

void IKEv2Connection_win::staticRasDialFunc(HRASCONN hRasConn, UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError)
{
    Q_ASSERT(this_ != NULL);
    this_->rasDialFuncCallback(hRasConn, unMsg, rascs, dwError, dwExtendedError);
}
