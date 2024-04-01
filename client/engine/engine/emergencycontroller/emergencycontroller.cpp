#include "emergencycontroller.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/connectionmanager/openvpnconnection.h"
#include "utils/hardcodedsettings.h"
#include <QFile>
#include <QCoreApplication>
#include "utils/extraconfig.h"
#include "types/global_consts.h"
#include <random>

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/helper/helper_mac.h"
#endif

using namespace wsnet;

EmergencyController::EmergencyController(QObject *parent, IHelper *helper) : QObject(parent),
    helper_(helper),
    state_(STATE_DISCONNECTED)
{
    connector_ = new OpenVPNConnection(this, helper_);
    connect(connector_, &OpenVPNConnection::connected, this, &EmergencyController::onConnectionConnected, Qt::QueuedConnection);
    connect(connector_, &OpenVPNConnection::disconnected, this, &EmergencyController::onConnectionDisconnected, Qt::QueuedConnection);
    connect(connector_, &OpenVPNConnection::reconnecting, this, &EmergencyController::onConnectionReconnecting, Qt::QueuedConnection);
    connect(connector_, &OpenVPNConnection::error, this, &EmergencyController::onConnectionError, Qt::QueuedConnection);

    makeOVPNFile_ = new MakeOVPNFile();
}

EmergencyController::~EmergencyController()
{
    if (request_) {
        request_->cancel();
    }
    SAFE_DELETE(connector_);
    SAFE_DELETE(makeOVPNFile_);
}

void EmergencyController::clickConnect(const types::ProxySettings &proxySettings, bool isAntiCensorship)
{
    WS_ASSERT(state_ == STATE_DISCONNECTED);
    state_= STATE_CONNECTING_FROM_USER_CLICK;

    proxySettings_ = proxySettings;
    isAntiCensorship_ = isAntiCensorship;


    auto callback = [this](std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> endpoints) {
        QMetaObject::invokeMethod(this, [this, endpoints]
        {
            endpoints_ = endpoints;
            doConnect();
        });
    };
    request_ = WSNet::instance()->emergencyConnect()->getIpEndpoints(callback);
}

void EmergencyController::clickDisconnect()
{
    WS_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED  ||
             state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_DISCONNECTED);

    if (request_) {
        request_->cancel();
        request_.reset();
    }

    if (state_ != STATE_DISCONNECTING_FROM_USER_CLICK)
    {
        state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
        qCDebug(LOG_EMERGENCY_CONNECT) << "ConnectionManager::clickDisconnect()";
        if (connector_)
        {
            connector_->startDisconnect();
        }
        else
        {
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
        }
    }
}

bool EmergencyController::isDisconnected()
{
    if (connector_)
    {
        return connector_->isDisconnected();
    }
    else
    {
        return true;
    }
}

void EmergencyController::blockingDisconnect()
{
    if (connector_)
    {
        if (!connector_->isDisconnected())
        {
            connector_->blockSignals(true);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();
            connector_->startDisconnect();
            while (!connector_->isDisconnected())
            {
                QThread::msleep(1);
                qApp->processEvents();

                if (elapsedTimer.elapsed() > 10000)
                {
                    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::blockingDisconnect() delay more than 10 seconds";
                    connector_->startDisconnect();
                    break;
                }
            }
            connector_->blockSignals(false);
            doMacRestoreProcedures();
            state_ = STATE_DISCONNECTED;
        }
    }
}

const AdapterGatewayInfo &EmergencyController::getVpnAdapterInfo() const
{
    WS_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return vpnAdapterInfo_;
}

void EmergencyController::setPacketSize(types::PacketSize ps)
{
    packetSize_ = ps;
}

void EmergencyController::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;
    qCDebug(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    state_ = STATE_CONNECTED;
    emit connected();
}

void EmergencyController::onConnectionDisconnected()
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionDisconnected(), state_ =" << state_;

    doMacRestoreProcedures();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_ERROR_DURING_CONNECTION:
            if (!endpoints_.empty())
            {
                state_ = STATE_CONNECTING_FROM_USER_CLICK;
                doConnect();
            }
            else
            {
                emit errorDuringConnection(CONNECT_ERROR::EMERGENCY_FAILED_CONNECT);
                state_ = STATE_DISCONNECTED;
            }
            break;
        default:
            WS_ASSERT(false);
    }
}

void EmergencyController::onConnectionReconnecting()
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionReconnecting(), state_ =" << state_;

    switch (state_)
    {
        case STATE_CONNECTED:
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
            state_ = STATE_ERROR_DURING_CONNECTION;
            connector_->startDisconnect();
            break;
        default:
            WS_ASSERT(false);
    }
}

void EmergencyController::onConnectionError(CONNECT_ERROR err)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionError(), err =" << err;

    connector_->startDisconnect();
    if (err == CONNECT_ERROR::AUTH_ERROR
            || err == CONNECT_ERROR::CANT_RUN_OPENVPN
            || err == CONNECT_ERROR::NO_OPENVPN_SOCKET
            || err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP
            || err == CONNECT_ERROR::ALL_TAP_IN_USE)
    {
        // emit error in disconnected event
        state_ = STATE_ERROR_DURING_CONNECTION;
    }
    else if (err == CONNECT_ERROR::UDP_CANT_ASSIGN
             || err == CONNECT_ERROR::UDP_NO_BUFFER_SPACE
             || err == CONNECT_ERROR::UDP_NETWORK_DOWN
             || err == CONNECT_ERROR::WINTUN_OVER_CAPACITY
             || err == CONNECT_ERROR::TCP_ERROR
             || err == CONNECT_ERROR::CONNECTED_ERROR
             || err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
    {
        if (state_ == STATE_CONNECTED)
        {
            state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
            connector_->startDisconnect();
        }
        else
        {
            state_ = STATE_ERROR_DURING_CONNECTION;
            connector_->startDisconnect();
        }
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Unknown error from openvpn: " << err;
    }
}

void EmergencyController::doConnect()
{
    defaultAdapterInfo_ = AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo();
    qCDebug(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();

    WS_ASSERT(!endpoints_.empty());
    auto endpoint = endpoints_[0];
    endpoints_.erase(endpoints_.begin());

    int mss = 0;
    if (!packetSize_.isAutomatic)
    {
        bool advParamsOpenVpnExists = false;
        int openVpnOffset = ExtraConfig::instance().getMtuOffsetOpenVpn(advParamsOpenVpnExists);
        if (!advParamsOpenVpnExists) openVpnOffset = MTU_OFFSET_OPENVPN;

        mss = packetSize_.mtu - openVpnOffset;

        if (mss <= 0)
        {
            mss = 0;
            qCDebug(LOG_PACKET_SIZE) << "Using default MSS - OpenVpn EmergencyController MSS too low: " << mss;
        }
        else
        {
            qCDebug(LOG_PACKET_SIZE) << "OpenVpn EmergencyController MSS: " << mss;
        }
    }
    else
    {
        qCDebug(LOG_PACKET_SIZE) << "Packet size mode auto - using default MSS (EmergencyController)";
    }

    QString protocol;
    if (endpoint->protocol() == Protocol::kTcp) {
        protocol = "TCP";
    } else if (endpoint->protocol() == Protocol::kUdp) {
        protocol = "UDP";
    } else {
        WS_ASSERT(false);
        return;
    }

    QString ovpnConfig = QString::fromStdString(WSNet::instance()->emergencyConnect()->ovpnConfig());
    WS_ASSERT(!ovpnConfig.isEmpty());

    bool bOvpnSuccess = makeOVPNFile_->generate(ovpnConfig, QString::fromStdString(endpoint->ip()), types::Protocol::fromString(protocol),
                                                endpoint->port(), 0, mss, defaultAdapterInfo_.gateway(), "", "", isAntiCensorship_);
    if (!bOvpnSuccess )
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Failed create ovpn config";
        WS_ASSERT(false);
        return;
    }

    qCDebug(LOG_EMERGENCY_CONNECT) << "Connecting to IP:" << QString::fromStdString(endpoint->ip()) << " protocol:" << protocol << " port:" << endpoint->port();

    QString username = QString::fromStdString(WSNet::instance()->emergencyConnect()->username());
    QString password = QString::fromStdString(WSNet::instance()->emergencyConnect()->password());

    if (!username.isEmpty() && !password.isEmpty()) {
        connector_->startConnect(makeOVPNFile_->config(), "", "", username, password, proxySettings_, nullptr, false, false, false, QString());
        lastIp_ = QString::fromStdString(endpoint->ip());
    } else {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Emergency credentials are empty";
        WS_ASSERT(false);
        return;
    }
}

void EmergencyController::doMacRestoreProcedures()
{
#ifdef Q_OS_MAC
    Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
    helper_mac->deleteRoute(lastIp_, 32, defaultAdapterInfo_.gateway());
    RestoreDNSManager_mac::restoreState(helper_);
#endif
}
