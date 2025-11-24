#pragma once

#include <map>
#include "../baserequest.h"

namespace wsnet {

class BridgeAPIRequest : public BaseRequest
{
public:
    explicit BridgeAPIRequest(HttpMethod requestType, SubdomainType subDomainType, const std::string &name,
                                    std::map<std::string, std::string> extraParams, const std::string &sessionToken,
                                    RequestFinishedCallback callback);
    virtual ~BridgeAPIRequest() {};

    virtual std::string url(const std::string &domain) const override;
    virtual std::string postData() const override;
    virtual void handle(const std::string &arr) override;

    void setSessionToken(const std::string &token);
    std::string sessionToken() const;

protected:
    virtual std::string hostname(const std::string &domain, SubdomainType subdomain) const override;

    const int kRequestTimeoutMs = 5000;

    std::string sessionToken_;
};

} // namespace wsnet
