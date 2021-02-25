#include "emergencycontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/connectionmanager/openvpnconnection.h"
#include "engine/hardcodedsettings.h"
#include "engine/dnsresolver/dnsresolver.h"
#include <QFile>
#include <QCoreApplication>
#include "utils/extraconfig.h"

#include <random>

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

EmergencyController::EmergencyController(QObject *parent, IHelper *helper) : QObject(parent),
    helper_(helper),
    serverApiUserRole_(0),
    state_(STATE_DISCONNECTED)
{
    QFile file(":/Resources/ovpn/emergency.ovpn");
    if (file.open(QIODevice::ReadOnly))
    {
        ovpnConfig_ = file.readAll();
        file.close();
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Failed load emergency.ovpn from resources";
    }

     connector_ = new OpenVPNConnection(this, helper_);
     connect(connector_, SIGNAL(connected(AdapterGatewayInfo)), SLOT(onConnectionConnected(AdapterGatewayInfo)), Qt::QueuedConnection);
     connect(connector_, SIGNAL(disconnected()), SLOT(onConnectionDisconnected()), Qt::QueuedConnection);
     connect(connector_, SIGNAL(reconnecting()), SLOT(onConnectionReconnecting()), Qt::QueuedConnection);
     connect(connector_, SIGNAL(error(CONNECTION_ERROR)), SLOT(onConnectionError(CONNECTION_ERROR)), Qt::QueuedConnection);

     makeOVPNFile_ = new MakeOVPNFile();

     connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void *)), SLOT(onDnsResolved(QString, QHostInfo,void *)));
}

EmergencyController::~EmergencyController()
{
    SAFE_DELETE(connector_);
    SAFE_DELETE(makeOVPNFile_);
}

void EmergencyController::clickConnect(const ProxySettings &proxySettings)
{
    Q_ASSERT(state_ == STATE_DISCONNECTED);
    state_= STATE_CONNECTING_FROM_USER_CLICK;

    proxySettings_ = proxySettings;

    QString hashedDomain = HardcodedSettings::instance().generateRandomDomain("econnect.");
    qCDebug(LOG_EMERGENCY_CONNECT) << "Generated hashed domain for emergency connect:" << hashedDomain;
    DnsResolver::instance().lookup(hashedDomain, this);
}

void EmergencyController::clickDisconnect()
{
    Q_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED  ||
             state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_DISCONNECTED);

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
            DnsResolver::instance().recreateDefaultDnsChannel();
            state_ = STATE_DISCONNECTED;
        }
    }
}

const AdapterGatewayInfo &EmergencyController::getVpnAdapterInfo() const
{
    Q_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return vpnAdapterInfo_;
}

void EmergencyController::setPacketSize(ProtoTypes::PacketSize ps)
{
    packetSize_ = ps;
}

void EmergencyController::onDnsResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    Q_UNUSED(hostname);
    if (userPointer == this)
    {

        attempts_.clear();

        if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
        {
            qCDebug(LOG_EMERGENCY_CONNECT) << "DNS resolved:" << hostInfo.addresses();

            // generate connect attempts array
            std::vector<QString> randomVecIps;
            for (int i = 0; i < hostInfo.addresses().count(); ++i)
            {
                randomVecIps.push_back(hostInfo.addresses()[i].toString());
            }

            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(randomVecIps.begin(), randomVecIps.end(), rd);

            for (std::vector<QString>::iterator it = randomVecIps.begin(); it != randomVecIps.end(); ++it)
            {
                CONNECT_ATTEMPT_INFO info1;
                info1.ip = *it;
                info1.port = 443;
                info1.protocol = "udp";

                CONNECT_ATTEMPT_INFO info2;
                info2.ip = *it;
                info2.port = 443;
                info2.protocol = "tcp";

                attempts_ << info1;
                attempts_ << info2;
            }

            addRandomHardcodedIpsToAttempts();
        }
        else
        {
            qCDebug(LOG_EMERGENCY_CONNECT) << "DNS resolve failed";
            addRandomHardcodedIpsToAttempts();
        }
        doConnect();
    }
}

void EmergencyController::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;
    qCDebug(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    DnsResolver::instance().recreateDefaultDnsChannel();

    state_ = STATE_CONNECTED;
    emit connected();
}

void EmergencyController::onConnectionDisconnected()
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionDisconnected(), state_ =" << state_;

    doMacRestoreProcedures();
    DnsResolver::instance().recreateDefaultDnsChannel();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_ERROR_DURING_CONNECTION:
            if (!attempts_.empty())
            {
                state_ = STATE_CONNECTING_FROM_USER_CLICK;
                doConnect();
            }
            else
            {
                emit errorDuringConnection(EMERGENCY_FAILED_CONNECT);
                state_ = STATE_DISCONNECTED;
            }
            break;
        default:
            Q_ASSERT(false);
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
            Q_ASSERT(false);
    }
}

void EmergencyController::onConnectionError(CONNECTION_ERROR err)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionError(), err =" << err;

    connector_->startDisconnect();
    if (err == AUTH_ERROR || err == CANT_RUN_OPENVPN || err == NO_OPENVPN_SOCKET ||
        err == NO_INSTALLED_TUN_TAP || err == ALL_TAP_IN_USE)
    {
        // emit error in disconnected event
        state_ = STATE_ERROR_DURING_CONNECTION;
    }
    else if (err == UDP_CANT_ASSIGN || err == UDP_NO_BUFFER_SPACE || err == UDP_NETWORK_DOWN || err == WINTUN_OVER_CAPACITY || err == TCP_ERROR ||
             err == CONNECTED_ERROR || err == INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
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

        // bIgnoreConnectionErrors_ need to prevent handle multiple error messages from openvpn
        /*if (!bIgnoreConnectionErrors_)
        {
            bIgnoreConnectionErrors_ = true;

            if (!checkFails())
            {
                if (state_ != STATE_RECONNECTING)
                {
                    emit reconnecting();
                    state_ = STATE_RECONNECTING;
                    startReconnectionTimer();
                }
                if (err == INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                {
                    bNeedResetTap_ = true;
                }
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                emit showFailedAutomaticConnectionMessage();
            }
        }*/
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Unknown error from openvpn: " << err;
    }
}

void EmergencyController::doConnect()
{
    defaultAdapterInfo_ = AdapterGatewayInfo::detectAndCreateDefaultAdaperInfo();
    qCDebug(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();

    Q_ASSERT(!attempts_.empty());
    CONNECT_ATTEMPT_INFO attempt = attempts_[0];
    attempts_.removeFirst();

    int mss = 0;
    if (!packetSize_.is_automatic())
    {
        bool advParamsOpenVpnExists = false;
        int openVpnOffset = ExtraConfig::instance().getMtuOffsetOpenVpn(advParamsOpenVpnExists);
        if (!advParamsOpenVpnExists) openVpnOffset = MTU_OFFSET_OPENVPN;

        mss = packetSize_.mtu() - openVpnOffset;

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


    bool bOvpnSuccess = makeOVPNFile_->generate(ovpnConfig_, attempt.ip, attempt.protocol, attempt.port, 0, mss, defaultAdapterInfo_.gateway());
    if (!bOvpnSuccess )
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Failed create ovpn config";
        Q_ASSERT(false);
        return;
    }

    qCDebug(LOG_EMERGENCY_CONNECT) << "Connecting to IP:" << attempt.ip << " protocol:" << attempt.protocol << " port:" << attempt.port;
    connector_->startConnect(makeOVPNFile_->path(), "", "", HardcodedSettings::instance().emergencyUsername(), HardcodedSettings::instance().emergencyPassword(), proxySettings_, nullptr, false, false);
    lastIp_ = attempt.ip;
}

void EmergencyController::doMacRestoreProcedures()
{
#ifdef Q_OS_MAC
    QString delRouteCommand = "route -n delete " + lastIp_ + "/32 " + defaultAdapterInfo_.gateway();
    qCDebug(LOG_EMERGENCY_CONNECT) << "Execute command: " << delRouteCommand;
    QString cmdAnswer = helper_->executeRootCommand(delRouteCommand);
    qCDebug(LOG_EMERGENCY_CONNECT) << "Output from route delete command: " << cmdAnswer;
    RestoreDNSManager_mac::restoreState(helper_);
#endif
}

void EmergencyController::addRandomHardcodedIpsToAttempts()
{
    QStringList ips = HardcodedSettings::instance().emergencyIps();
    std::vector<QString> randomVecIps;
    Q_FOREACH(const QString &ip, ips)
    {
        randomVecIps.push_back(ip);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(randomVecIps.begin(), randomVecIps.end(), rd);

    for (std::vector<QString>::iterator it = randomVecIps.begin(); it != randomVecIps.end(); ++it)
    {
        CONNECT_ATTEMPT_INFO info;
        info.ip = *it;
        info.port = 1194;
        info.protocol = "udp";

        attempts_ << info;
    }
}
