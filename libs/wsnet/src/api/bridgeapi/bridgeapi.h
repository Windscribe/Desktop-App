#pragma once

#include "WSNetBridgeAPI.h"
#include <boost/asio.hpp>
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include "utils/persistentsettings.h"
#include "connectstate.h"

namespace wsnet {

class BridgeAPI_impl;

class BridgeAPI : public WSNetBridgeAPI
{
public:
    explicit BridgeAPI(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState);
    virtual ~BridgeAPI();

    void setConnectedState(bool isConnected) override;
    void setIgnoreSslErrors(bool bIgnore) override;
    bool hasSessionToken() const override;
    void setApiAvailableCallback(WSNetApiAvailableCallback callback) override;
    void setCurrentHost(const std::string &host) override;

    std::shared_ptr<WSNetCancelableCallback> pinIp(const std::string &ip, WSNetRequestFinishedCallback callback) override;
    std::shared_ptr<WSNetCancelableCallback> rotateIp(WSNetRequestFinishedCallback callback) override;

private:
    std::unique_ptr<BridgeAPI_impl> impl_;
    boost::asio::io_context &io_context_;
    PersistentSettings &persistentSettings_;
    WSNetAdvancedParameters *advancedParameters_;
    ConnectState &connectState_;
    std::uint32_t subscriberId_;

    void onVPNConnectStateChanged(bool isConnected);
};

} // namespace wsnet
