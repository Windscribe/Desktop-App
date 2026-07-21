#include "ws_branding.h"
#include "ikev2connection_win.h"

#include <QElapsedTimer>

#include "engine/connectionmanager/adapterutils_win.h"
#include "engine/connectionmanager/iextraconfigaccessor.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "utils/ras_service_win.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

#define IKEV2_CONNECTION_NAME WS_WIN_IKEV2_CONNECTION_NAME_W

namespace {
// Payload carried from the RAS callback (RAS-owned thread) to the worker thread via QueueUserAPC.
struct DialStatePayload
{
    IKEv2Connection_win *self;
    HRASCONN hrasconn;
    RASCONNSTATE rascs;
    DWORD dwError;
};
}

// static global variable, because we reinstall WAN only once during the lifetime of the process
bool IKEv2Connection_win::wanReinstalled_ = false;

IKEv2Connection_win::IKEv2Connection_win(QObject *parent, Helper *helper, types::Protocol protocol, const Ikev2SessionParams &sessionParams)
    : Ikev2ConnectionBase(parent, protocol, sessionParams),
      helper_(helper)
{
    // stopThreadEvent_ is manual-reset so it stays signaled until the worker observes and resets it on the
    // next connect.  notifyEvent_ is auto-reset, matching RasConnectionNotification's re-arm model.
    stopThreadEvent_.setHandle(::CreateEvent(NULL, TRUE, FALSE, NULL));
    notifyEvent_.setHandle(::CreateEvent(NULL, FALSE, FALSE, NULL));
}

IKEv2Connection_win::~IKEv2Connection_win()
{
    // Ensure the worker thread (and thus any RAS callback queued to it) has fully stopped before this
    // instance is destroyed.
    if (isRunning()) {
        stop();
        wait();
    }
}

void IKEv2Connection_win::startConnect()
{
    WS_ASSERT(helper_ != nullptr);
    WS_ASSERT(stopThreadEvent_.isValid() && notifyEvent_.isValid());

    // Defensive: if a previous worker is somehow still running, stop and join it first.
    if (isRunning()) {
        stop();
        wait();
    }

    initialUrl_ = descr_.hostname;
    initialIp_ = descr_.ip;
    initialUsername_ = username();
    initialPassword_ = password();
    initialEnableIkev2Compression_ = env_.extraConfig->useIkev2Compression();

    ::ResetEvent(stopThreadEvent_.getHandle());

    start(LowPriority);
}

void IKEv2Connection_win::startDisconnect()
{
    if (isDisconnected()) {
        // No worker running (never connected, or already torn down).  Surface a disconnected event so
        // the engine's state machine completes -- this mirrors the engine driving a disconnect after a
        // terminal connect error.
        emit disconnected();
    } else {
        stop();
    }
}

bool IKEv2Connection_win::isDisconnected() const
{
    return !isRunning();
}

void IKEv2Connection_win::waitForDisconnect()
{
    wait();
}

void IKEv2Connection_win::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    connectedSignalEmited_ = false;
    cntFailedConnectionAttempts_ = 0;
    hostsAndDnsProtectionSet_ = false;
    connHandle_ = NULL;

    // Duplicate a real handle to this worker thread so the RAS callback (which runs on a RAS-owned
    // thread) can QueueUserAPC back to us.  Held for the lifetime of the instance and closed by the
    // destructor; by the time run() returns, RasHangUp has completed and no further callbacks fire.
    HANDLE dup = NULL;
    if (::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &dup, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        workerThreadHandle_.setHandle(dup);

        // Attempt loop: re-entered only when a WAN reinstall/enable recovery wants to retry the dial.
        for (;;) {
            pendingResult_ = PendingResult::None;

            const DialResult dr = startDial();
            if (dr == DialResult::Error || dr == DialResult::Stopped) {
                break;
            }

            const MonitorResult res = monitorLoop();

            if (res == MonitorResult::ReinstallWanRequested) {
                const ReinstallResult rr = handleReinstallWan();
                if (rr == ReinstallResult::Retry) {
                    continue;
                }
                break;
            }

            // Terminal paths (Stop / Dropped / AuthError / Failed) share common cleanup.  Any dial-state
            // APCs still queued from the teardown are drained/freed (and ignored) by handleDialState when
            // blockingDisconnect's alertable wait runs them: a user Stop is seen via stopRequested(); the
            // AuthError/Failed paths already latched pendingResult_; and Dropped only happens post-connect,
            // where connectedSignalEmited_ blocks any re-emit.
            blockingDisconnect(connHandle_, true);
            connHandle_ = NULL;
            cleanupHostsAndDnsProtection();

            if (res == MonitorResult::AuthError) {
                emit error(CONNECT_ERROR::AUTH_ERROR);
            } else if (res == MonitorResult::Failed) {
                // Drop detection could not be armed for the just-established tunnel or a transient connection failure occurred.
                // Fail over rather than give up.  The engine drives its own disconnect for this (non-auth) error, but the extra
                // disconnected() below is harmless -- see the teardown note at the tail.
                emit error(CONNECT_ERROR::IKEV_FAILED_TO_CONNECT);
            }
            break;
        }
    } else {
        qCCritical(LOG_IKEV2) << "DuplicateHandle for worker thread failed:" << ::GetLastError();
        emit error(CONNECT_ERROR::IKEV_FAILED_TO_CONNECT);
    }

    // Single exit: run() tears the tunnel down on every path above, so a terminal disconnected() is always
    // correct here -- this mirrors WireGuardConnection::run().  Where the engine also drives a disconnect in
    // response to an error() above (e.g. the IKEV_FAILED_TO_CONNECT failover calls startDisconnect(), which
    // self-emits), the extra disconnected() is de-duplicated by ConnectionManager::onConnectionDisconnected()
    // via its sender()/SAFE_DELETE_LATER guard.
    emit disconnected();
}

void IKEv2Connection_win::stop()
{
    if (stopThreadEvent_.isValid()) {
        BOOL result = ::SetEvent(stopThreadEvent_.getHandle());
        if (result == FALSE) {
            qCCritical(LOG_CONNECTION) << "IKEv2Connection_win::stop - SetEvent failed:" << ::GetLastError();
        }
    }
}

bool IKEv2Connection_win::stopRequested() const
{
    return stopThreadEvent_.isSignaled();
}

IKEv2Connection_win::DialResult IKEv2Connection_win::startDial()
{
    // A disconnect requested during this synchronous dial setup is only observed at these checkpoints,
    // between the blocking steps -- a single in-flight blocking call still has to finish before we bail.
    // cleanupHostsAndDnsProtection() is guarded, so it is a no-op at the early checkpoints.
    if (stopRequested()) {
        return DialResult::Stopped;
    }

    // Clean up any stale connections left over from a previous crash or abnormal termination.  If a
    // stale "Windscribe IKEv2" RAS connection exists, RasSetEntryProperties can crash with an access
    // violation inside the RAS API due to orphaned internal state.
    const QVector<HRASCONN> staleConns = getActiveIkev2Connections();
    for (HRASCONN h : staleConns) {
        if (stopRequested()) {
            return DialResult::Stopped;
        }
        qCInfo(LOG_IKEV2) << "Hanging up stale IKEv2 connection";
        blockingDisconnect(h, true);
    }
    if (!staleConns.isEmpty()) {
        removeIkev2ConnectionFromOS();
    }

    bool ikev2DeviceInitialized = false;
    RASDEVINFO devInfo;
    if (!findIKEv2Device(&devInfo)) {
        qCInfo(LOG_IKEV2) << "Trying restart SstpSvc and RasMan";
        if (!RAS_Service_win::instance().restartRASServices(helper_)) {
            qCInfo(LOG_IKEV2) << "Failed to start SstpSvc and/or RasMan services";
        } else {
            qCInfo(LOG_IKEV2) << "SstpSvc and/or RasMan services restarted";
            ikev2DeviceInitialized = findIKEv2Device(&devInfo);
            if (!ikev2DeviceInitialized) {
                qCWarning(LOG_IKEV2) << "getIKEv2Device failed again";
            }
        }
    } else {
        ikev2DeviceInitialized = true;
    }

    if (!ikev2DeviceInitialized) {
        emit error(CONNECT_ERROR::IKEV_NOT_FOUND_WIN);
        return DialResult::Error;
    }

    if (stopRequested()) {
        return DialResult::Stopped;
    }

    RASENTRY rasEntry;
    memset(&rasEntry, 0, sizeof(rasEntry));
    rasEntry.dwSize = sizeof(RASENTRY);

    // Use the truncating form (_TRUNCATE) so an over-long source string returns a non-zero errno instead
    // of invoking the secure-CRT invalid-parameter handler, which terminates the process.  szLocalPhoneNumber
    // is fed by the server-supplied hostname, so fail (rather than silently truncate) if it doesn't fit.
    errno_t ret = wcsncpy_s(rasEntry.szLocalPhoneNumber, qUtf16Printable(initialUrl_), _TRUNCATE);
    if (ret != 0) {
        qCCritical(LOG_IKEV2) << "Failed to copy hostname to RAS phone number field, error:" << ret;
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }
    // szDeviceName/szDeviceType come from RasEnumDevices into identically-sized fields, so they cannot
    // overflow; use the truncating form anyway for consistency.
    wcsncpy_s(rasEntry.szDeviceName, devInfo.szDeviceName, _TRUNCATE);
    wcsncpy_s(rasEntry.szDeviceType, devInfo.szDeviceType, _TRUNCATE);

    rasEntry.dwfOptions = RASEO_RequireEAP  /*| RASEO_RemoteDefaultGateway*/;
    if (initialEnableIkev2Compression_) {
        rasEntry.dwfOptions = rasEntry.dwfOptions | RASEO_IpHeaderCompression | RASEO_SwCompression;
    }

    // Note: RASEO2_DisableMobility is intentionally NOT set, so MOBIKE stays enabled (the SA can follow
    // an external IP change).
    rasEntry.dwfOptions2 = RASEO2_DontNegotiateMultilink | RASEO2_IPv4ExplicitMetric | RASEO2_IPv6ExplicitMetric | RASEO2_SecureFileAndPrint | RASEO2_ReconnectIfDropped;
    rasEntry.dwFramingProtocol = RASFP_Ppp;
    rasEntry.dwEncryptionType = ET_RequireMax;
    rasEntry.dwType = RASET_Vpn;
    rasEntry.dwVpnStrategy = VS_Ikev2Only;
    rasEntry.dwfNetProtocols = RASNP_Ip | RASNP_Ipv6;
    rasEntry.dwRedialCount = 3;
    rasEntry.dwRedialPause = 60;
    rasEntry.dwIPv4InterfaceMetric = 1;  // minimum ipv4 metric
    rasEntry.dwIPv6InterfaceMetric = 1;  // minimum ipv6 metric
    rasEntry.dwIdleDisconnectSeconds = RASIDS_Disabled;
    // dwNetworkOutageTime is the ceiling, in minutes, on how long Windows keeps a dormant tunnel alive
    // (retransmitting IKE packets) before declaring it dead.  If connectivity returns within that window
    // -- same or different interface -- MOBIKE re-attaches the same SA with no re-dial, no re-auth, and no
    // app-visible reconnect.
    rasEntry.dwNetworkOutageTime = kNetworkOutageTimeMinutes;

    DWORD result = RasSetEntryProperties(NULL, IKEV2_CONNECTION_NAME, &rasEntry, rasEntry.dwSize, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        qCCritical(LOG_IKEV2) << "RasSetEntryProperties failed with error:" << result;
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }

    RASDIALPARAMS dialparams;
    memset(&dialparams, 0, sizeof(dialparams));
    dialparams.dwSize = sizeof(dialparams);
    dialparams.dwCallbackId = (ULONG_PTR)this;

    // szEntryName is a branded compile-time constant and cannot overflow; the username and password are
    // session credentials fed in from outside, so fail if they don't fit rather than silently truncate.
    wcsncpy_s(dialparams.szEntryName, IKEV2_CONNECTION_NAME, _TRUNCATE);
    ret = wcsncpy_s(dialparams.szUserName, qUtf16Printable(initialUsername_), _TRUNCATE);
    if (ret != 0) {
        qCCritical(LOG_IKEV2) << "Failed to copy username to RAS dial params, error:" << ret;
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }
    ret = wcsncpy_s(dialparams.szPassword, qUtf16Printable(initialPassword_), _TRUNCATE);
    if (ret != 0) {
        qCCritical(LOG_IKEV2) << "Failed to copy password to RAS dial params, error:" << ret;
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }

    result = RasSetEntryDialParams(NULL, &dialparams, FALSE);
    if (result != ERROR_SUCCESS) {
        qCCritical(LOG_IKEV2) << "RasSetEntryDialParams failed with error:" << result;
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }

    if (stopRequested()) {
        return DialResult::Stopped;
    }

    if (!helper_->addHosts(initialIp_, initialUrl_)) {
        qCCritical(LOG_IKEV2) << "Can't modify hosts file";
        emit error(CONNECT_ERROR::IKEV_FAILED_MODIFY_HOSTS_WIN);
        return DialResult::Error;
    }

    helper_->setIKEv2IPSecParameters();
    helper_->enableDnsLeaksProtection();
    hostsAndDnsProtectionSet_ = true;

    if (stopRequested()) {
        cleanupHostsAndDnsProtection();
        return DialResult::Stopped;
    }

    connHandle_ = NULL;

    // Asynchronous dial: RasDial returns immediately and reports progress through rasDialCallbackProc.
    result = RasDial(NULL, NULL, &dialparams, 2, (LPVOID)rasDialCallbackProc, &connHandle_);
    if (result != ERROR_SUCCESS) {
        qCCritical(LOG_IKEV2) << "RasDial failed with error:" << result;
        // RasDial may have stored a handle even on failure; always hang it up.
        blockingDisconnect(connHandle_, true);
        connHandle_ = NULL;
        cleanupHostsAndDnsProtection();
        emit error(CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN);
        return DialResult::Error;
    }

    return DialResult::Started;
}

IKEv2Connection_win::MonitorResult IKEv2Connection_win::monitorLoop()
{
    HANDLE waits[2] = { stopThreadEvent_.getHandle(), notifyEvent_.getHandle() };

    for (;;) {
        // Alertable so RAS dial-state APCs (queued by rasDialCallbackProc) run on this thread.
        const DWORD result = ::WaitForMultipleObjectsEx(2, waits, FALSE, kTimeoutForGetStats, TRUE);

        if (result == WAIT_OBJECT_0) {
            // stopThreadEvent_ -- user/engine requested disconnect.
            return MonitorResult::Stop;
        } else if (result == WAIT_OBJECT_0 + 1) {
            // notifyEvent_ -- RasConnectionNotification fired.  It only says "something changed"; confirm
            // the tunnel is actually down before reporting a drop, and otherwise re-arm.
            if (isRasDisconnected(connHandle_)) {
                qCWarning(LOG_IKEV2) << "RasConnectionNotification indicates the connection has dropped";
                return MonitorResult::Dropped;
            }
            // Re-arm for the next event.  If we can no longer arm drop detection we have lost the
            // ability to notice a future disconnect, so treat it as a drop.
            if (!armDropDetection()) {
                return MonitorResult::Dropped;
            }
        } else if (result == WAIT_IO_COMPLETION) {
            // A dial-state APC ran on this thread (handleDialState).  It applies the connected transition
            // inline; for terminal outcomes it sets pendingResult_ for us to act on here.
            if (pendingResult_ == PendingResult::AuthError) {
                return MonitorResult::AuthError;
            }
            if (pendingResult_ == PendingResult::ReinstallWan) {
                return MonitorResult::ReinstallWanRequested;
            }
            if (pendingResult_ == PendingResult::ConnectFailed) {
                return MonitorResult::Failed;
            }
        } else if (result == WAIT_TIMEOUT) {
            emitStatistics();
        } else {
            qCCritical(LOG_IKEV2) << "WaitForMultipleObjectsEx failed:" << ::GetLastError();
            return MonitorResult::Dropped;
        }
    }
}

bool IKEv2Connection_win::armDropDetection()
{
    // Without drop detection we cannot notice a future disconnect, so callers treat an arming failure
    // as a connection failure rather than proceeding silently.
    const DWORD err = RasConnectionNotification(connHandle_, notifyEvent_.getHandle(), RASCN_Disconnection);
    if (err != ERROR_SUCCESS) {
        qCWarning(LOG_IKEV2) << "RasConnectionNotification failed to arm drop detection:" << err;
        return false;
    }
    return true;
}

IKEv2Connection_win::ReinstallResult IKEv2Connection_win::handleReinstallWan()
{
    blockingDisconnect(connHandle_, true);
    connHandle_ = NULL;
    cleanupHostsAndDnsProtection();

    // The user/engine may have requested a disconnect while the teardown above was running (notably
    // blockingDisconnect, which can block for several seconds).  Skip the heavyweight, system-wide WAN
    // miniport reinstall in that case -- local cleanup is already done above, so the caller's Stopped
    // path emits the terminal disconnected() without re-doing it.
    if (stopRequested()) {
        return ReinstallResult::Stopped;
    }

    // With issues 498 and 576, it was found that only restarting the app would rectify the AuthNotify
    // error.  Removing the connection here helps with those (non-reproducible) issues and does no harm.
    removeIkev2ConnectionFromOS();

    cntFailedConnectionAttempts_++;
    if (cntFailedConnectionAttempts_ >= kMaxFailedConnectionAttempts) {
        emit error(CONNECT_ERROR::IKEV_FAILED_TO_CONNECT);
        return ReinstallResult::TerminalError;
    }

    if (helper_->isWanIkev2AdapterDisabled()) {
        qCInfo(LOG_IKEV2) << "WAN Miniport (IKEv2) or WAN Miniport (IP) disabled, trying to enable it";
        helper_->enableWanIkev2();
    } else if (!wanReinstalled_) {
        helper_->reinstallWanIkev2();
        wanReinstalled_ = true;
        qCInfo(LOG_IKEV2) << "Reinstalled Wan IKEv2";
    } else {
        emit error(CONNECT_ERROR::IKEV_FAILED_TO_CONNECT);
        return ReinstallResult::TerminalError;
    }

    // Pause before re-dialing, but remain responsive to a disconnect request during the wait.
    if (stopThreadEvent_.wait(kWanReinstallPause) == WAIT_OBJECT_0) {
        return ReinstallResult::Stopped;
    }
    return ReinstallResult::Retry;
}

void IKEv2Connection_win::emitStatistics()
{
    if (connHandle_ == NULL || !connectedSignalEmited_) {
        return;
    }
    RAS_STATS stats;
    stats.dwSize = sizeof(RAS_STATS);
    if (RasGetLinkStatistics(connHandle_, 1, &stats) == ERROR_SUCCESS) {
        emit statisticsUpdated(stats.dwBytesRcved, stats.dwBytesXmited, false);
        RasClearLinkStatistics(connHandle_, 1);
    }
}

void IKEv2Connection_win::cleanupHostsAndDnsProtection()
{
    if (hostsAndDnsProtectionSet_) {
        helper_->disableDnsLeaksProtection();
        helper_->removeHosts();
        hostsAndDnsProtectionSet_ = false;
    }
}

bool IKEv2Connection_win::isTransientConnectError(DWORD dwError)
{
    // Dial errors that indicate a server/network-side problem (the peer is unreachable or the
    // IKE negotiation timed out) rather than a broken local WAN miniport.  These warrant a plain
    // reconnect / engine failover instead of the heavyweight WAN reinstall recovery.  The list is
    // intentionally an allow-list: any unrecognized error falls through to the reinstall path.
    switch (dwError) {
    case ERROR_DISCONNECTION:
    case ERROR_REMOTE_DISCONNECTION:
    case ERROR_AUTHENTICATION_FAILURE:
    case ERROR_VPN_TIMEOUT:
    case ERROR_IPSEC_IKE_TIMED_OUT:         // 13805 - IKE negotiation timed out (peer did not respond)
    case ERROR_IPSEC_IKE_DROP_NO_RESPONSE:  // 13813 - no response from peer
    case ERROR_NETWORK_UNREACHABLE:         // 1231
    case ERROR_HOST_UNREACHABLE:            // 1232
        return true;
    default:
        return false;
    }
}

void IKEv2Connection_win::handleDialState(HRASCONN connHandle, RASCONNSTATE rascs, DWORD dwError)
{
    // Ignore possible stale APCs if the handle for the APC does not match our current RAS handle.
    if (connHandle != connHandle_) {
        qCWarning(LOG_IKEV2) << "Skipping stale RAS callback due to RAS handle mismatch";
        return;
    }

    // Runs on the worker thread (via QueueUserAPC), so it may freely touch worker-thread state and call
    // helper_/RAS and emit signals.
    const QString str = rasConnStateToString(rascs);
    if (dwError == ERROR_SUCCESS) {
        qCInfo(LOG_IKEV2) << "RasDial state:" << str;
    } else {
        wchar_t strErr[1024];
        DWORD result = ::RasGetErrorString(dwError, strErr, _countof(strErr));
        if (result != ERROR_SUCCESS) {
            // RasGetErrorString fails for non-RAS error codes (e.g. ERROR_IPSEC_IKE_AUTH_FAIL).
            WinUtils::Win32GetErrorString(dwError, strErr, _countof(strErr));
        }
        qCInfo(LOG_IKEV2) << "RasDial state:" << str << "Error code:" << dwError << QString::fromWCharArray(strErr);
    }

    // Ignore progress once the user/engine has requested a disconnect, or once a terminal decision is
    // already pending (the AuthError/ConnectFailed/ReinstallWan paths latch pendingResult_ until the next
    // attempt).  A post-connect Dropped teardown reaches here with neither set, but connectedSignalEmited_
    // then blocks the only harmful action (a re-emit of connected()).
    if (stopRequested() || pendingResult_ != PendingResult::None) {
        return;
    }

    if (dwError == ERROR_SUCCESS) {
        // Only the first transition into the connected state is interesting; ignore any repeats so we
        // never emit connected() twice.
        if (rascs == RASCS_Connected && !connectedSignalEmited_) {
            // Arm asynchronous drop detection for the established tunnel.  If we cannot arm it we have
            // no way to detect a future drop, so fail the connection instead of reporting it connected.
            if (!armDropDetection()) {
                pendingResult_ = PendingResult::ConnectFailed;
                return;
            }
            helper_->addIKEv2DefaultRoute();
            // A concurrent stop() on the engine thread may have signaled stopThreadEvent_ while we armed
            // drop detection / added the route above.  If so, abandon the connected transition rather than
            // emitting a connected() the engine never expects after asking to disconnect; run()'s stop path
            // tears the tunnel down regardless.
            if (!stopRequested()) {
                connectedSignalEmited_ = true;
                AdapterGatewayInfo cai = AdapterUtils_win::getConnectedAdapterInfo(QString::fromWCharArray(IKEV2_CONNECTION_NAME));
                emit connected(cai);
            }
        }
    } else {
        if (rascs == RASCS_AuthNotify && dwError == ERROR_AUTHENTICATION_FAILURE) {  // Error 691
            pendingResult_ = PendingResult::AuthError;
        } else if (!connectedSignalEmited_) {
            if (isTransientConnectError(dwError)) {
                // Server/network-side failure (peer unreachable or IKE negotiation timed out): the local
                // WAN miniport is fine, so report a connect failure and let the engine retry / fail over
                // rather than burning the once-per-process WAN reinstall on it.
                pendingResult_ = PendingResult::ConnectFailed;
            } else {
                // Other non-auth errors may indicate a broken/disabled WAN miniport; run the WAN
                // reinstall/enable recovery.
                pendingResult_ = PendingResult::ReinstallWan;
            }
        }
    }
}

void CALLBACK IKEv2Connection_win::rasDialApc(ULONG_PTR param)
{
    std::unique_ptr<DialStatePayload> payload((DialStatePayload*)param);
    if (payload && payload->self) {
        payload->self->handleDialState(payload->hrasconn, payload->rascs, payload->dwError);
    }
}

DWORD CALLBACK IKEv2Connection_win::rasDialCallbackProc(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hrasconn,
                                                        UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError)
{
    IKEv2Connection_win *pThis = (IKEv2Connection_win*)dwCallbackId;
    if (pThis == NULL) {
        qCWarning(LOG_IKEV2) << "Skipping RAS callback due to null instance pointer for RAS state" << rascs << "error" << dwError;
        return FALSE;
    }

    if (pThis->workerThreadHandle_.isValid()) {
        DialStatePayload *payload = new DialStatePayload{ pThis, hrasconn, rascs, dwError };
        const auto result = ::QueueUserAPC(&IKEv2Connection_win::rasDialApc, pThis->workerThreadHandle_.getHandle(), (ULONG_PTR)payload);
        if (result == 0) {
            qCWarning(LOG_IKEV2) << "QueueUserAPC failed:" << ::GetLastError();
            delete payload;
        }
    }

    return TRUE;
}

void IKEv2Connection_win::removeIkev2ConnectionFromOS()
{
    DWORD dwErr = RasDeleteEntry(NULL, IKEV2_CONNECTION_NAME);
    if (dwErr != ERROR_SUCCESS && dwErr != ERROR_CANNOT_FIND_PHONEBOOK_ENTRY) {
        qCWarning(LOG_IKEV2) << "RasDeleteEntry failed with error:" << dwErr;
    }
}

QVector<HRASCONN> IKEv2Connection_win::getActiveIkev2Connections()
{
    QVector<HRASCONN> v;

    DWORD dwCb = 0;
    DWORD dwConnections = 0;
    LPRASCONN lpRasConn = NULL;

    DWORD result = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);
    if (result == ERROR_BUFFER_TOO_SMALL) {
        QByteArray arr(dwCb, Qt::Uninitialized);
        lpRasConn = (LPRASCONN)arr.data();
        lpRasConn[0].dwSize = sizeof(RASCONN);
        result = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);
        if (result == ERROR_SUCCESS) {
            for (DWORD i = 0; i < dwConnections; i++) {
                if (wcscmp(lpRasConn[i].szEntryName, IKEV2_CONNECTION_NAME) == 0) {
                    v << lpRasConn[i].hrasconn;
                }
            }
        } else {
            qCWarning(LOG_IKEV2) << "IKEv2Connection_win::getActiveIkev2Connections() - RasEnumConnections failed with error:" << result;
        }
    } else if (result != ERROR_SUCCESS) {
        qCWarning(LOG_IKEV2) << "IKEv2Connection_win::getActiveIkev2Connections() - initial RasEnumConnections failed with error:" << result;
    }

    return v;
}

void IKEv2Connection_win::blockingDisconnect(HRASCONN connHandle)
{
    // Public entry point: non-alertable, safe from any thread.
    blockingDisconnect(connHandle, false);
}

void IKEv2Connection_win::blockingDisconnect(HRASCONN connHandle, bool alertable)
{
    if (connHandle == NULL) {
        return;
    }

    DWORD result = RasHangUp(connHandle);
    if (result == ERROR_INVALID_HANDLE || result == ERROR_NO_CONNECTION) {
        return;
    }

    qCInfo(LOG_IKEV2) << "IKEv2Connection_win::blockingDisconnect() - RasHangUp return code:" << result;

    QElapsedTimer elapsedTimer;
    QElapsedTimer totalTimer;
    elapsedTimer.start();
    totalTimer.start();
    int cntRasHangUp = 1;

    // Design Note: kBlockingDisconnectTimeoutMs is aligned to be less than the timeout used by the engine
    // in ConnectionManager::blockingDisconnect to ensure we exit here before the engine bails and starts
    // tearing the connector down.
    while (totalTimer.elapsed() < kBlockingDisconnectTimeoutMs) {
        if (isRasDisconnected(connHandle)) {
            qCInfo(LOG_IKEV2) << "IKEv2Connection_win::blockingDisconnect() - RasGetConnectStatus indicates we're disconnected";
            return;
        }

        if (elapsedTimer.elapsed() > kRasHangUpRetryIntervalMs && cntRasHangUp < kMaxRasHangUpAttempts) {
            result = RasHangUp(connHandle);
            elapsedTimer.restart();
            qCInfo(LOG_IKEV2) << "IKEv2Connection_win::blockingDisconnect() - calling RasHangUp again:" << result;
            cntRasHangUp++;
        }

        // When called on the worker thread (alertable=true), run any dial-state APCs queued by RAS during
        // the hang-up (freeing their payloads) rather than leaking them when the thread exits.  Other
        // callers stay non-alertable so they don't run unrelated APCs queued to their thread.
        ::SleepEx(kDisconnectPollIntervalMs, alertable ? TRUE : FALSE);
    }

    qCInfo(LOG_IKEV2) << "IKEv2Connection_win::blockingDisconnect() timed out waiting for disconnect";
}

bool IKEv2Connection_win::findIKEv2Device(RASDEVINFO *outDevInfo)
{
    RASDEVINFO *devInfo;
    DWORD dwSize = 0;
    DWORD dwNumberDevices = 0;

    DWORD result = RasEnumDevices(0, &dwSize, &dwNumberDevices);
    if (result != ERROR_BUFFER_TOO_SMALL && result != ERROR_SUCCESS) {
        qCCritical(LOG_IKEV2) << "RasEnumDevices failed with error:" << result;
        return false;
    }

    // It is possible for RasEnumDevices to return ERROR_SUCCESS and dwSize == 0, for example on
    // broken/missing RAS installs.
    if (dwSize < sizeof(RASDEVINFO)) {
        qCCritical(LOG_IKEV2) << "RasEnumDevices returned an invalid buffer size:" << dwSize;
        return false;
    }

    QByteArray arr;
    arr.resize(dwSize);
    devInfo = (RASDEVINFO*)arr.data();
    devInfo[0].dwSize = sizeof(RASDEVINFO);

    result = RasEnumDevices(devInfo, &dwSize, &dwNumberDevices);
    if (result == ERROR_SUCCESS) {
        QString strLog;

        for (DWORD i = 0; i < dwNumberDevices; ++i) {
            QString devtype = QString::fromWCharArray(devInfo[i].szDeviceType).toLower();
            QString devname = QString::fromWCharArray(devInfo[i].szDeviceName).toLower();

            strLog += devtype + ", " + devname + "; ";

            if (devtype.contains("vpn") && devname.contains("ikev2")) {
                if (outDevInfo) {
                    *outDevInfo = devInfo[i];
                }
                return true;
            }
        }

        qCInfo(LOG_IKEV2) << "An IKEv2 device was not found in the device list returned by RasEnumDevices:" << strLog;
    } else {
        qCCritical(LOG_IKEV2) << "RasEnumDevices failed with error:" << result;
        return false;
    }

    return false;
}

QString IKEv2Connection_win::rasConnStateToString(RASCONNSTATE state)
{
    switch (state) {
    case RASCS_OpenPort: return "OpenPort";
    case RASCS_PortOpened: return "PortOpened";
    case RASCS_ConnectDevice: return "ConnectDevice";
    case RASCS_DeviceConnected: return "DeviceConnected";
    case RASCS_AllDevicesConnected: return "AllDevicesConnected";
    case RASCS_Authenticate: return "Authenticate";
    case RASCS_AuthNotify: return "AuthNotify";
    case RASCS_AuthRetry: return "AuthRetry";
    case RASCS_AuthCallback: return "AuthCallback";
    case RASCS_AuthChangePassword: return "AuthChangePassword";
    case RASCS_AuthProject: return "AuthProject";
    case RASCS_AuthLinkSpeed: return "AuthLinkSpeed";
    case RASCS_AuthAck: return "AuthAck";
    case RASCS_ReAuthenticate: return "ReAuthenticate";
    case RASCS_Authenticated: return "Authenticated";
    case RASCS_PrepareForCallback: return "PrepareForCallback";
    case RASCS_WaitForModemReset: return "WaitForModemReset";
    case RASCS_WaitForCallback: return "WaitForCallback";
    case RASCS_Projected: return "Projected";
    case RASCS_StartAuthentication: return "StartAuthentication";
    case RASCS_CallbackComplete: return "CallbackComplete";
    case RASCS_LogonNetwork: return "LogonNetwork";
    case RASCS_SubEntryConnected: return "SubEntryConnected";
    case RASCS_SubEntryDisconnected: return "SubEntryDisconnected";
    case RASCS_ApplySettings: return "ApplySettings";
    case RASCS_Interactive: return "Interactive";
    case RASCS_RetryAuthentication: return "RetryAuthentication";
    case RASCS_CallbackSetByCaller: return "CallbackSetByCaller";
    case RASCS_PasswordExpired: return "PasswordExpired";
    case RASCS_InvokeEapUI: return "InvokeEapUI";
    case RASCS_Connected: return "Connected";
    case RASCS_Disconnected: return "Disconnected";
    default:
        WS_ASSERT(false);
        return "rasConnStateToString: unknown value";
    }
}

bool IKEv2Connection_win::isRasDisconnected(HRASCONN connHandle)
{
    RASCONNSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwSize = sizeof(status);
    
    const DWORD result = RasGetConnectStatus(connHandle, &status);
    if (result == ERROR_INVALID_HANDLE || result == ERROR_NO_CONNECTION) {
        qCInfo(LOG_IKEV2) << "RasGetConnectStatus indicated disconnected via error code:" << result;
        return true;
    } else if (result != ERROR_SUCCESS) {
        qCWarning(LOG_IKEV2) << "RasGetConnectStatus failed with error:" << result;
    }
    return false;
}
