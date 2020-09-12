#include "automanualconnectioncontroller.h"
#include "utils/logger.h"

#include <QDataStream>

AutoManualConnectionController::AutoManualConnectionController() :
    curAttempt_(0), failedIkev2Counter_(0), isFailedIkev2CounterAlreadyIncremented_(false),
    failedManualModeCounter_(0), bIsAllFailed_(false), bStarted_(false)
{
}

/*void AutoManualConnectionController::startWith(QSharedPointer<MutableLocationInfo> mli, const ConnectionSettings &connectionSettings,
                                               const PortMap &portMap, bool isProxyEnabled)
{
    attemps_.clear();
    bStarted_ = true;
    curAttempt_ = 0;
    failedManualModeCounter_ = 0;
    bIsAllFailed_ = false;
    isFailedIkev2CounterAlreadyIncremented_ = false;
    connectionSettings_ = connectionSettings;
    portMap_ = portMap;
    mli_ = mli;

    // remove wstunnel protocol from portMap_ for automatic connection mode
    if (connectionSettings_.isAutomatic() && !mli_->isCustomOvpnConfig())
    {
        QVector<PortItem>::iterator it = portMap_.items.begin();
        while (it != portMap_.items.end())
        {
            if (it->protocol.getType() == ProtocolType::PROTOCOL_WSTUNNEL)
            {
                it = portMap_.items.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }


    if (connectionSettings_.isAutomatic() && !mli_->isCustomOvpnConfig())
    {
        // sort portmap protocols in the following order: ikev2, udp, tcp, stealth
        qSort(portMap_.items.begin(), portMap_.items.end(), sortPortMapFunction);

        ProtocolType lastSuccessProtocolSaved;
        QSettings settings;
        if (settings.contains("successConnectionProtocol"))
        {
            QByteArray arr = settings.value("successConnectionProtocol").toByteArray();
            QDataStream stream(&arr, QIODevice::ReadOnly);
            QString strProtocol;
            stream >> strProtocol;
            lastSuccessProtocolSaved = ProtocolType(strProtocol);
        }

        for (int nodeInd = 0; nodeInd < mli_->nodesCount(); ++nodeInd)
        {
            QVector<AttemptInfo> localAttemps;
            for (int portMapInd = 0; portMapInd < portMap_.items.count(); ++portMapInd)
            {
                // skip udp protocol, if proxy enabled
                if (isProxyEnabled && portMap_.items[portMapInd].protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_UDP)
                {
                    continue;
                }
                // skip ikev2 protocol if failed ikev2 attempts >= MAX_IKEV2_FAILED_ATTEMPTS
                if (failedIkev2Counter_ >= MAX_IKEV2_FAILED_ATTEMPTS && portMap_.items[portMapInd].protocol.isIkev2Protocol())
                {
                    continue;
                }

                AttemptInfo attemptInfo;
                attemptInfo.protocol = portMap_.items[portMapInd].protocol;
                Q_ASSERT(portMap_.items[portMapInd].ports.count() > 0);
                attemptInfo.portMapInd = portMapInd;
                attemptInfo.changeNode = false;

                localAttemps << attemptInfo;
            }

            if (localAttemps.count() > 0)
            {
                localAttemps.last().changeNode = true;
            }

            // if we have successfully saved connection settings, then use it first (move on top list)
            // but if first protocol ikev2, then use it second
            if (lastSuccessProtocolSaved.isInitialized())
            {
                AttemptInfo firstAttemptInfo;
                bool bFound = false;
                for (int i = 0; i < localAttemps.count(); ++i)
                {
                    if (localAttemps[i].protocol.isEqual(lastSuccessProtocolSaved))
                    {
                        firstAttemptInfo = localAttemps[i];
                        localAttemps.remove(i);
                        bFound = true;
                        break;
                    }
                }
                if (bFound)
                {
                    if (localAttemps.count() > 0 && localAttemps.first().protocol.isIkev2Protocol())
                    {
                        localAttemps.insert(1, firstAttemptInfo);
                    }
                    else
                    {
                        localAttemps.insert(0, firstAttemptInfo);
                    }
                }
            }

            // copy sorted localAttemps to attemps_
            attemps_ << localAttemps;
        }

        // duplicate attempts (because we need do all attempts twice)
        attemps_ << attemps_;
    }
}*/

void AutoManualConnectionController::reset()
{
    if (connectionSettings_.isAutomatic())
    {
        curAttempt_ = 0;
        bIsAllFailed_ = false;
    }
}

void AutoManualConnectionController::stop()
{
    bStarted_ = false;
}

void AutoManualConnectionController::debugLocationInfoToLog()
{
    QString strNodes;
    for (int i = 0; i < mli_->nodesCount(); ++i)
    {
        strNodes += getLogForNode(i);
        strNodes += "; ";
    }

    connectionSettings_.logConnectionSettings();
    qCDebug(LOG_CONNECTION) << "Location nodes:" << strNodes;
}

QString AutoManualConnectionController::getLogForNode(int ind)
{
    QString ret = "node" + QString::number(ind + 1);

    //todo
    /*if (mli_->getNode(ind).isLegacy())
    {
        ret += "(legacy)";
    }
    ret += " = {";

    const int IPS_COUNT = 3;
    for (int i = 0; i < IPS_COUNT; ++i)
    {
        ret += "ip" + QString::number(i + 1) + " = ";
        ret += mli_->getNode(ind).getIp(i);
        if (i != (IPS_COUNT - 1))
        {
            ret += ", ";
        }
    }

    ret += "}";*/
    return ret;
}

// sort portmap protocols in the following order: ikev2, udp, tcp, stealth
// operator<
/*bool AutoManualConnectionController::sortPortMapFunction(const PortItem &p1, const PortItem &p2)
{
    if (p1.protocol.getType() == ProtocolType::PROTOCOL_IKEV2)
    {
        return true;
    }
    else if (p1.protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_UDP)
    {
        if (p2.protocol.getType() == ProtocolType::PROTOCOL_IKEV2 || p2.protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_UDP)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (p1.protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_TCP)
    {
        if (p2.protocol.getType() == ProtocolType::PROTOCOL_IKEV2 || p2.protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_UDP
                || p2.protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_TCP)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (p1.protocol.getType() == ProtocolType::PROTOCOL_STUNNEL)
    {
        return false;
    }
    else
    {
        Q_ASSERT(false);
    }
    return 0;
}*/

void AutoManualConnectionController::putFailedConnection()
{
    if (!bStarted_)
    {
        return;
    }

    if (connectionSettings_.isAutomatic() && !mli_->isCustomOvpnConfig())
    {
        if (attemps_[curAttempt_].protocol.isIkev2Protocol() && !isFailedIkev2CounterAlreadyIncremented_)
        {
            failedIkev2Counter_++;
            isFailedIkev2CounterAlreadyIncremented_ = true;
        }

        if (curAttempt_ < (attemps_.count() - 1))
        {
            if (attemps_[curAttempt_].changeNode)
            {
                mli_->selectNextNode();
            }
            curAttempt_++;
        }
        else
        {
            bIsAllFailed_ = true;
        }
    }
    else
    {
        if (failedManualModeCounter_ >= 2)
        {
            // try switch to another node for manual mode
            mli_->selectNextNode();
        }
        else
        {
            failedManualModeCounter_++;
        }
    }
}

bool AutoManualConnectionController::isFailed()
{
    if (!bStarted_)
    {
        return false;
    }

    if (connectionSettings_.isAutomatic() && !mli_->isCustomOvpnConfig())
    {
        return bIsAllFailed_;
    }
    else
    {
        return false;
    }
}

AutoManualConnectionController::CurrentConnectionDescr AutoManualConnectionController::getCurrentConnectionSettings()
{
    CurrentConnectionDescr ccd;
    /*if (!connectionSettings_.isInitialized())
    {
        qCDebug(LOG_CONNECTION) << "Fatal error, connectionSettings_ not initialized";
        Q_ASSERT(false);
        ccd.connectionNodeType = CONNECTION_NODE_ERROR;
        return ccd;
    }

    if (mli_->isCustomOvpnConfig())
    {
        ccd.connectionNodeType = CONNECTION_NODE_CUSTOM_OVPN_CONFIG;
        ccd.pathOvpnConfigFile = mli_->getCustomOvpnConfigPath();
        ccd.ip = mli_->getSelectedNode().getIp(0);
        ccd.protocol = ProtocolType::PROTOCOL_OPENVPN_UDP;
        ccd.port = 0;
    }
    else if (connectionSettings_.isAutomatic())
    {
        ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
        ccd.protocol = attemps_[curAttempt_].protocol;

        if (mli_->getSelectedNode().isLegacy())
        {
            ccd.port = portMap_.getLegacyPort(ccd.protocol);
        }
        else
        {
            ccd.port = portMap_.items[attemps_[curAttempt_].portMapInd].ports[0];
        }
        int useIpInd = portMap_.getUseIpInd(ccd.protocol);
        ccd.ip = mli_->getSelectedNode().getIp(useIpInd);
        ccd.hostname = mli_->getSelectedNode().getHostname();
        ccd.dnsHostName = mli_->getDnsName();
    }
    else
    {
        ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
        ccd.protocol = connectionSettings_.protocol();
        ccd.port = connectionSettings_.port();

        // adjust port for legacy nodes
        if (mli_->getSelectedNode().isLegacy())
        {
            ccd.port = portMap_.getLegacyPort(ccd.protocol);
        }

        int useIpInd = portMap_.getUseIpInd(connectionSettings_.protocol());
        ccd.ip = mli_->getSelectedNode().getIp(useIpInd);
        ccd.hostname = mli_->getSelectedNode().getHostname();
        ccd.dnsHostName = mli_->getDnsName();
    }
    if (mli_->isStaticIp())
    {
        ccd.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
        ccd.username = mli_->getStaticIpUsername();
        ccd.password = mli_->getStaticIpPassword();
        ccd.staticIpPorts = mli_->getStaticIpPorts();
    }*/
    return ccd;
}

bool AutoManualConnectionController::isCurrentIkev2Protocol()
{
    if (mli_->isCustomOvpnConfig())
    {
        return false;
    }
    else if (connectionSettings_.isAutomatic())
    {
        if (curAttempt_ >= 0 && curAttempt_ < attemps_.count())
        {
            return attemps_[curAttempt_].protocol.isIkev2Protocol();
        }
        else
        {
            return false;
        }
    }
    else
    {
        return connectionSettings_.protocol().isIkev2Protocol();
    }
}

void AutoManualConnectionController::saveCurrentSuccessfullConnectionSettings()
{
    if (connectionSettings_.isAutomatic() && !mli_->isCustomOvpnConfig())
    {
        // reset ikev2 failed attempts counter if we connected with ikev2 successfully
        if (attemps_[curAttempt_].protocol.isIkev2Protocol())
        {
            failedIkev2Counter_ = 0;
        }


        QSettings settings;

        QString protocol = attemps_[curAttempt_].protocol.toLongString();
        qCDebug(LOG_CONNECTION) << "Save latest successfully connection protocol:" << protocol;

        QByteArray arr;
        {
            QDataStream stream(&arr, QIODevice::WriteOnly);
            stream << protocol;
        }

        settings.setValue("successConnectionProtocol", arr);
    }
}

bool AutoManualConnectionController::isAutomaticMode()
{
    return connectionSettings_.isAutomatic();
}

void AutoManualConnectionController::autoTest()
{
    /*QVector<ServerNode> nodes;
    ConnectionSettings connectionSettings;
    PortMap portMap;

    PortItem p1;
    p1.heading = "UDP";
    p1.legacy_port = 443;
    p1.ports << 53 << 80 << 443 << 1194 << 54783;
    p1.protocol = ProtocolType("UDP");
    p1.use = "ip2";

    PortItem p2;
    p2.heading = "TCP";
    p2.legacy_port = 1194;
    p2.ports << 443 << 587 << 21 << 22 << 80 << 143 << 3306 << 8080 << 54783 << 1194;
    p2.protocol =  ProtocolType("TCP");
    p2.use = "ip2";

    PortItem p3;
    p3.heading = "Stealth";
    p3.legacy_port = 8443;
    p3.ports << 443 << 587 << 21 << 22 << 80 << 143 << 3306 << 8080 << 54783 << 8443;
    p3.protocol = ProtocolType("Stealth");
    p3.use = "ip3";

    PortItem p4;
    p4.heading = "IKEv2";
    p4.legacy_port = 0;
    p4.ports << 555;
    p4.protocol = ProtocolType("IKEv2");
    p4.use = "ip2";
    portMap.items << p1 << p2 << p3 << p4;


    ServerNode n1;
    n1.ip_[0] = "1.2.3.4";
    n1.ip_[1] = "2.3.4.5";
    n1.ip_[2] = "6.7.8.9";
    n1.isValid_ = true;
    n1.legacy_ = 1;

    ServerNode n2;
    n2.ip_[0] = "10.11.12.13";
    n2.ip_[1] = "14.15.16.17";
    n2.ip_[2] = "18.19.20.21";
    n2.isValid_ = true;
    n2.legacy_ = 0;

    nodes << n1 << n2;*/

    // test manual

    /*QSharedPointer<MutableLocationInfo> mli(new MutableLocationInfo(LocationID(23), "AAA", nodes, 0, "dnsHostname"));

    AutoManualConnectionController ac;
    connectionSettings.set(ProtocolType::PROTOCOL_IKEV2, 80, false);
    ac.startWith(mli, connectionSettings, portMap, false);
    Q_ASSERT(!ac.isAutomaticMode());

    ProtocolType protocol;
    QString ip, hostname, dnsHostname;
    uint port;
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.putFailedConnection();
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    int end;*/

    //test automatic
    /*QSharedPointer<MutableLocationInfo> mli(new MutableLocationInfo(LocationID(23), "AAA", nodes, 0, "dnsHostname"));
    AutoManualConnectionController ac;
    connectionSettings.set(ProtocolType::PROTOCOL_OPENVPN_UDP, 80, true);
    ac.startWith(mli, connectionSettings, portMap, false);
    Q_ASSERT(ac.isAutomaticMode());
    ProtocolType protocol;
    QString ip, hostname, dnsHostname;
    uint port;

    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();

    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();

    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();
    ac.getCurrentConnectionSettings(protocol, ip, port, hostname, dnsHostname);
    ac.putFailedConnection();*/
}
