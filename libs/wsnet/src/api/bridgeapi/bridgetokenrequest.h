#pragma once

#include "bridgeapi_request.h"

namespace wsnet {

class BridgeTokenRequest : public BridgeAPIRequest
{
public:
    explicit BridgeTokenRequest(const std::string &sessionToken, RequestFinishedCallback callback);
    virtual ~BridgeTokenRequest() {};

    virtual void handle(const std::string &arr) override;

    const std::string& getSessionToken() const;

private:
    std::string sessionTokenResponse_;
};

} // namespace wsnet