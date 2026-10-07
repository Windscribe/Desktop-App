#include "bridgeapimanager.h"

#include <wsnet/WSNet.h>
#include <wsnet/WSNetBridgeAPI.h>

#include "utils/log/categories.h"

using namespace wsnet;

BridgeApiManager::BridgeApiManager(QObject *parent) : QObject(parent), isConnected_(false)
{
    WSNet::instance()->bridgeAPI()->setApiAvailableCallback([this](bool isAvailable) {
        emit apiAvailabilityChanged(isAvailable);
    });
}

BridgeApiManager::~BridgeApiManager()
{
}

void BridgeApiManager::setConnectedState(bool isConnected, int nodeId, const types::Protocol &protocol, const QString &pinnedIp)
{
    if (isConnected == isConnected_) {
        return;
    }

    qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::setConnectedState";

    isConnected_ = isConnected;

    if (!isConnected_) {
        return;
    }

    // Always call setCurrentNodeId when connecting, even with no node (e.g. custom configs), to clear
    // the previous connection's node in wsnet and prevent token fetches for the wrong server. The id
    // and not the hostname: a bridge token belongs to one node, and every node in a datacenter can
    // answer to the same hostname. Do not use cache for non-Wireguard protocols.
    WSNet::instance()->bridgeAPI()->setCurrentNodeId(protocol.isWireGuardProtocol() ? nodeId : 0);

    if (!pinnedIp.isEmpty()) {
        pinIp(pinnedIp);
    }
}

void BridgeApiManager::rotateIp()
{
    if (!isConnected_) {
        return;
    }

    qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::rotateIp";

    // wsnet handles all retries/timeouts/operations internally
    WSNet::instance()->bridgeAPI()->rotateIp([this](wsnet::ApiRetCode ret, const std::string &jsonData) {
        if (ret == wsnet::ApiRetCode::kSuccess) {
            qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::rotateIp success";
            // Server does not provide the new IP, we just have to do tunnel test again to get it.
            emit ipRotateFinished(true);
        } else if (ret == wsnet::ApiRetCode::kNoToken) {
            qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::rotateIp failed: no session token";
            emit ipRotateFinished(false);
        } else {
            qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::rotateIp failed";
            emit ipRotateFinished(false);
        }
    });
}

void BridgeApiManager::pinIp(const QString &ip)
{
    if (!isConnected_) {
        return;
    }

    qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::pinIp" << ip;

    WSNet::instance()->bridgeAPI()->pinIp(ip.toStdString(), [this, ip](wsnet::ApiRetCode ret, const std::string &jsonData) {
        if (ret == wsnet::ApiRetCode::kSuccess) {
            qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::pinIp success";
            emit ipPinFinished(ip);
        } else {
            if (ret == wsnet::ApiRetCode::kNoToken) {
                qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::pinIp failed: no session token";
            } else {
                qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::pinIp failed";
            }
            emit ipPinFinished(QString());
        }
    });
}
