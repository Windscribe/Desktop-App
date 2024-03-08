#pragma once

#include <map>
#include "WSNetAdvancedParameters.h"
#include "baserequest.h"
#include "serverapi_settings.h"
#include "connectstate.h"

namespace wsnet {

// This request is different from all the others, so we make a class with overriding behavior
// In particular, the country override logic needs to be executed here
// Also, do not pass client_auth_hash, platform, version parameters for this request
class ServerLocationsRequest : public BaseRequest
{
public:
    explicit ServerLocationsRequest(RequestPriority priority, const std::string &name,
                                    std::map<std::string, std::string> extraParams, ServerAPISettings &settings,
                                    ConnectState &connectState, WSNetAdvancedParameters *advancedParameters, RequestFinishedCallback callback);
    virtual ~ServerLocationsRequest() {};

    std::string url(const std::string &domain) const override;
    void handle(const std::string &arr) override;


private:
    ServerAPISettings &settings_;
    mutable bool isFromDisconnectedVPNState_;
    ConnectState &connectState_;
    WSNetAdvancedParameters *advancedParameters_;
};

} // namespace wsnet

