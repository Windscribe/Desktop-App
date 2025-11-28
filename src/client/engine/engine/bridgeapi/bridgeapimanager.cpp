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

void BridgeApiManager::setConnectedState(bool isConnected, const QString &nodeAddress, const types::Protocol &protocol, const QString &pinnedIp)
{
    if (isConnected == isConnected_) {
        return;
    }

    qCDebug(LOG_BRIDGE_API) << "BridgeApiManager::setConnectedState";

    isConnected_ = isConnected;

    if (!isConnected_ || nodeAddress.isEmpty()) {
        return;
    }

    // Do not use cache for non-Wireguard protocols
    WSNet::instance()->bridgeAPI()->setCurrentHost(protocol.isWireGuardProtocol() ? nodeAddress.toStdString() : "");

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
