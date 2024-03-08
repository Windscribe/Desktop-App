#pragma once

#include <map>
#include "baserequest.h"

namespace wsnet {

// This request is different from all the others, so we make a class with overriding behavior
class SetRobertFilterRequest : public BaseRequest
{
public:
    explicit SetRobertFilterRequest(HttpMethod requestType, SubdomainType subDomainType, RequestPriority priority, const std::string &name,
                                    std::map<std::string, std::string> extraParams, const std::string &jsonPostData,
                                    RequestFinishedCallback callback);
    virtual ~SetRobertFilterRequest() {};

    std::string url(const std::string &domain) const override;
    std::string postData() const override;

private:
    std::string jsonPostData_;
};

} // namespace wsnet

