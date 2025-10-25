#pragma once

#include <functional>
#include <memory>
#include <string>
#include "scapix_object.h"
#include "WSNetCancelableCallback.h"
#include "WSNetServerAPI.h"

namespace wsnet {

using WSNetApiAvailableCallback = std::function<void(bool isAvailable)>;

class WSNetBridgeAPI : public scapix_object<WSNetBridgeAPI>
{
public:
    virtual ~WSNetBridgeAPI() {}

    virtual void setConnectedState(bool isConnected) = 0;
    virtual void setIgnoreSslErrors(bool bIgnore) = 0;
    virtual bool hasSessionToken() const = 0;
    virtual void setApiAvailableCallback(WSNetApiAvailableCallback callback) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> rotateIp(WSNetRequestFinishedCallback callback) = 0;
    virtual std::shared_ptr<WSNetCancelableCallback> pinIp(const std::string &ip, WSNetRequestFinishedCallback callback) = 0;
};

} // namespace wsnet
