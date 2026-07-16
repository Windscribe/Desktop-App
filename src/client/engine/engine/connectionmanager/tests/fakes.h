#pragma once

#include <QPointer>

#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/connectionmanager/connectors/iconnectionfactory.h"
#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/connectionmanager/connsettingspolicy/iconnsettingspolicyfactory.h"
#include "engine/connectionmanager/isleepevents.h"
#include "engine/connectionmanager/iextraconfigaccessor.h"
#include "engine/helper/ihelperbackend.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"

// Test doubles for every ConnectionManager seam. They never spin threads or touch the OS.

class FakeHelperBackend : public IHelperBackend
{
    Q_OBJECT
public:
    explicit FakeHelperBackend(QObject *parent = nullptr) : IHelperBackend(parent) {}

    void startInstallHelper(bool /*bForceDeleteOld*/) override {}
    State currentState() const override { return State::kConnected; }
    bool reinstallHelper() override { return true; }
    std::string sendCmd(int /*cmdId*/, const std::string & /*data*/) override { return std::string(); }
};

// Minimal concrete BaseLocationInfo: ConnectionManager only checks that lastRequest_.bli is
// non-null before building a policy, so nothing here needs real node data.
class FakeLocationInfo : public locationsmodel::BaseLocationInfo
{
    Q_OBJECT
public:
    FakeLocationInfo() : BaseLocationInfo(LocationID(), "fake") {}
    bool isExistSelectedNode() const override { return true; }
    QString getVerifyX509name() const override { return QString(); }
    QString getLogString() const override { return QString(); }
};

// A connector that never spins a thread. startConnect/startDisconnect just record intent; the
// connector-side signals ConnectionManager listens to are emitted on demand via the drive* methods
// so tests can pin how CM reacts to each. Signals reach CM through Qt::QueuedConnection, so tests
// must pump the event loop (QTRY_*/processEvents) after driving one.
class FakeConnection : public IConnection
{
    Q_OBJECT
public:
    FakeConnection(types::Protocol protocol, QObject *parent) : IConnection(parent), protocol_(protocol) {}

    void startConnect(const StartConnectParams & /*params*/) override
    {
        isDisconnected_ = false;
        startConnectCount_++;
    }
    void startDisconnect() override
    {
        startDisconnectCount_++;
        // Real connectors confirm the stop asynchronously via disconnected(); model that by default so
        // CM's post-disconnect transitions run. Tests that need to observe the intermediate state turn
        // this off and drive disconnected() themselves.
        if (autoEmitDisconnected_) {
            isDisconnected_ = true;
            emit disconnected();
        }
    }
    bool isDisconnected() const override { return isDisconnected_; }
    // The Windows sleep path in blockingDisconnect() pumps no events and relies on this blocking
    // until the stop completes; model that completion.
    void waitForDisconnect() override { isDisconnected_ = true; }
    ConnectionType getConnectionType() const override
    {
        if (protocol_.isWireGuardProtocol()) {
            return ConnectionType::WIREGUARD;
        } else if (protocol_.isIkev2Protocol()) {
            return ConnectionType::IKEV2;
        }
        return ConnectionType::OPENVPN;
    }

    void continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/) override
    {
        continueWithUsernameAndPasswordCount_++;
    }
    void continueWithPassword(const QString & /*password*/) override { continueWithPasswordCount_++; }

    // Connector-side events CM reacts to.
    void driveConnected(const AdapterGatewayInfo &info = AdapterGatewayInfo())
    {
        isDisconnected_ = false;
        emit connected(info);
    }
    void driveDisconnected()
    {
        isDisconnected_ = true;
        emit disconnected();
    }
    void driveReconnecting() { emit reconnecting(); }
    void driveError(CONNECT_ERROR err) { emit error(err); }
    void driveStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
    {
        emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
    }
    void driveInterfaceUpdated(const QString &interfaceName) { emit interfaceUpdated(interfaceName); }
    void driveRequestUsername() { emit requestUsername(); }
    void driveRequestPassword() { emit requestPassword(); }

    void setAutoEmitDisconnected(bool value) { autoEmitDisconnected_ = value; }

    int startConnectCount() const { return startConnectCount_; }
    int startDisconnectCount() const { return startDisconnectCount_; }
    int continueWithUsernameAndPasswordCount() const { return continueWithUsernameAndPasswordCount_; }
    int continueWithPasswordCount() const { return continueWithPasswordCount_; }

private:
    types::Protocol protocol_;
    bool isDisconnected_ = true;
    bool autoEmitDisconnected_ = true;
    int startConnectCount_ = 0;
    int startDisconnectCount_ = 0;
    int continueWithUsernameAndPasswordCount_ = 0;
    int continueWithPasswordCount_ = 0;
};

class FakeConnectionFactory : public IConnectionFactory
{
public:
    IConnection *createConnection(types::Protocol protocol, QObject *parent, Helper * /*helper*/) override
    {
        createdCount_++;
        lastCreated_ = new FakeConnection(protocol, parent);
        return lastCreated_;
    }
    void removeIkev2ConnectionFromOS() override { removeIkev2Count_++; }

    // CM retires connectors with deleteLater(), so this stays non-null (retired but alive) until the
    // event loop runs; snapshot it before triggering a reconnect if comparing old vs new.
    FakeConnection *lastCreated() const { return lastCreated_.data(); }
    int createdCount() const { return createdCount_; }
    int removeIkev2Count() const { return removeIkev2Count_; }

private:
    QPointer<FakeConnection> lastCreated_;
    int createdCount_ = 0;
    int removeIkev2Count_ = 0;
};

class FakePlatformPolicy : public IConnectionPlatformPolicy
{
public:
    FakePlatformPolicy()
    {
        // Non-empty by default so doConnect() proceeds past the network-connectivity gate.
        defaultAdapter_.setAdapterName("fake0");
        defaultAdapter_.addGatewayIp(types::IpAddress("192.168.1.1"));
    }

    bool isLockdownBlockingIkev2() const override { return isLockdownBlockingIkev2_; }
    void disableDohIfNeeded() override { disableDohCount_++; }
    void setGaiIpv4PriorityEnabled(bool isEnabled) override { lastGaiIpv4Priority_ = isEnabled; }
    AdapterGatewayInfo detectDefaultAdapter() override { return defaultAdapter_; }

    void setLockdownBlockingIkev2(bool blocking) { isLockdownBlockingIkev2_ = blocking; }
    void setDefaultAdapter(const AdapterGatewayInfo &info) { defaultAdapter_ = info; }
    int disableDohCount() const { return disableDohCount_; }
    bool lastGaiIpv4Priority() const { return lastGaiIpv4Priority_; }

private:
    bool isLockdownBlockingIkev2_ = false;
    AdapterGatewayInfo defaultAdapter_;
    int disableDohCount_ = 0;
    bool lastGaiIpv4Priority_ = false;
};

// Defaults match an absent ExtraConfig file: no overrides, errors honored, empty IKEv2 remote. Tests
// override only the value the case exercises.
class FakeExtraConfig : public IExtraConfigAccessor
{
public:
    bool logCtrld() override { return false; }
    int mtuOffsetOpenVpn(bool &hasValue) override { hasValue = hasMtuOffsetOpenVpn_; return mtuOffsetOpenVpn_; }
    bool stealthExtraTLSPadding() override { return stealthExtraTLSPadding_; }
    QString remoteIp() override { return remoteIp_; }
    QString detectedIp() override { return detectedIp_; }
    bool useIkev2Compression() override { return useIkev2Compression_; }
    int tunnelTestAttempts(bool &hasValue) override { hasValue = hasTunnelTestAttempts_; return tunnelTestAttempts_; }
    bool isTunnelTestNoError() override { return tunnelTestNoError_; }

    void setMtuOffsetOpenVpn(bool hasValue, int offset) { hasMtuOffsetOpenVpn_ = hasValue; mtuOffsetOpenVpn_ = offset; }
    void setStealthExtraTLSPadding(bool value) { stealthExtraTLSPadding_ = value; }
    void setRemoteIp(const QString &ip) { remoteIp_ = ip; }
    void setDetectedIp(const QString &ip) { detectedIp_ = ip; }
    void setUseIkev2Compression(bool value) { useIkev2Compression_ = value; }
    void setTunnelTestAttempts(bool hasValue, int attempts) { hasTunnelTestAttempts_ = hasValue; tunnelTestAttempts_ = attempts; }
    void setTunnelTestNoError(bool noError) { tunnelTestNoError_ = noError; }

private:
    bool hasMtuOffsetOpenVpn_ = false;
    int mtuOffsetOpenVpn_ = 0;
    bool stealthExtraTLSPadding_ = false;
    QString remoteIp_;
    QString detectedIp_;
    bool useIkev2Compression_ = false;
    bool hasTunnelTestAttempts_ = false;
    int tunnelTestAttempts_ = 0;
    bool tunnelTestNoError_ = false;
};

class FakeConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    void reset() override { resetCount_++; }
    void debugLocationInfoToLog() const override {}
    void putFailedConnection() override { putFailedConnectionCount_++; }
    bool isFailed() const override { return isFailed_; }
    CurrentConnectionDescr getCurrentConnectionSettings() const override { return currentConnectionDescr_; }
    bool isAutomaticMode() override { return isAutomaticMode_; }
    bool isCustomConfig() override { return isCustomConfig_; }
    void resolveHostnames() override
    {
        // Real policies resolve then emit hostnamesResolved(), which (same thread, direct connection)
        // synchronously drives doConnectPart2/3. Tests that want CM parked in the connecting state
        // before prep runs turn this off and drive the relevant slot directly.
        if (autoResolveHostnames_) {
            emit hostnamesResolved();
        }
    }
    bool hasProtocolChanged() override { return hasProtocolChanged_; }

    void setCurrentConnectionSettings(const CurrentConnectionDescr &descr) { currentConnectionDescr_ = descr; }
    void setFailed(bool failed) { isFailed_ = failed; }
    void setAutomaticMode(bool automatic) { isAutomaticMode_ = automatic; }
    void setCustomConfig(bool customConfig) { isCustomConfig_ = customConfig; }
    void setProtocolChanged(bool changed) { hasProtocolChanged_ = changed; }
    void setAutoResolveHostnames(bool value) { autoResolveHostnames_ = value; }
    int resetCount() const { return resetCount_; }
    int putFailedConnectionCount() const { return putFailedConnectionCount_; }

private:
    CurrentConnectionDescr currentConnectionDescr_;
    bool isFailed_ = false;
    bool isAutomaticMode_ = false;
    bool isCustomConfig_ = false;
    bool hasProtocolChanged_ = false;
    bool autoResolveHostnames_ = true;
    int resetCount_ = 0;
    int putFailedConnectionCount_ = 0;
};

class FakeConnSettingsPolicyFactory : public IConnSettingsPolicyFactory
{
public:
    BaseConnSettingsPolicy *createPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> /*bli*/,
                                         const types::ConnectionSettings & /*connectionSettings*/,
                                         const api_responses::PortMap & /*portMap*/,
                                         const types::ProxySettings & /*proxySettings*/,
                                         const QString & /*preferredNodeHostname*/,
                                         bool /*isFirewallAlwaysOnPlusEnabled*/,
                                         bool /*hasUsableCachedWgConfig*/) override
    {
        // Preserve the caller-configured policy across CM's repeated updateConnectionSettingsPolicy()
        // calls (clickConnect + every failover recreates one). Tests seed template_ and read back the
        // live instance via lastCreated().
        lastCreated_ = new FakeConnSettingsPolicy();
        lastCreated_->setCurrentConnectionSettings(templateDescr_);
        lastCreated_->setAutomaticMode(isAutomaticMode_);
        lastCreated_->setCustomConfig(isCustomConfig_);
        lastCreated_->setFailed(isFailed_);
        lastCreated_->setProtocolChanged(hasProtocolChanged_);
        lastCreated_->setAutoResolveHostnames(autoResolveHostnames_);
        return lastCreated_;
    }

    FakeConnSettingsPolicy *lastCreated() const { return lastCreated_.data(); }

    // Template applied to every policy this factory hands out.
    void setCurrentConnectionSettings(const CurrentConnectionDescr &descr) { templateDescr_ = descr; }
    void setAutomaticMode(bool automatic) { isAutomaticMode_ = automatic; }
    void setCustomConfig(bool customConfig) { isCustomConfig_ = customConfig; }
    void setFailed(bool failed) { isFailed_ = failed; }
    void setProtocolChanged(bool changed) { hasProtocolChanged_ = changed; }
    void setAutoResolveHostnames(bool value) { autoResolveHostnames_ = value; }

private:
    QPointer<FakeConnSettingsPolicy> lastCreated_;
    CurrentConnectionDescr templateDescr_;
    bool isAutomaticMode_ = false;
    bool isCustomConfig_ = false;
    bool isFailed_ = false;
    bool hasProtocolChanged_ = false;
    bool autoResolveHostnames_ = true;
};

// CM owns the ISleepEvents it is handed (setParent + SAFE_DELETE); tests keep a raw pointer only to
// drive the OS sleep/wake signals.
class FakeSleepEvents : public ISleepEvents
{
    Q_OBJECT
public:
    explicit FakeSleepEvents(QObject *parent = nullptr) : ISleepEvents(parent) {}
    void driveSleep() { emit gotoSleep(); }
    void driveWake() { emit gotoWake(); }
};

class FakeNetworkDetectionManager : public INetworkDetectionManager
{
    Q_OBJECT
public:
    explicit FakeNetworkDetectionManager(QObject *parent = nullptr) : INetworkDetectionManager(parent) {}

    void getCurrentNetworkInterface(types::NetworkInterface & /*networkInterface*/, bool /*forceUpdate*/) override {}
    bool isOnline() override { return isOnline_; }

    void setOnline(bool isOnline)
    {
        if (isOnline_ != isOnline) {
            isOnline_ = isOnline;
            emit onlineStateChanged(isOnline_);
        }
    }

private:
    bool isOnline_ = true;
};
