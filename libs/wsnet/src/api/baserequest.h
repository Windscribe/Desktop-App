#pragma once

#include <map>
#include "WSNetHttpRequest.h"
#include "WSNetServerAPI.h"
#include "utils/cancelablecallback.h"
#include "settings.h"

namespace wsnet {

enum class SubdomainType { kApi, kAssets, kBridgeAPI };
enum class RequestPriority { kNormal, kHigh };

using RequestFinishedCallback = std::shared_ptr<CancelableCallback<WSNetRequestFinishedCallback>>;


struct ApiOverrideSettings
{
    std::string apiRoot;
    std::string assetsRoot;

    bool isOverriden() const
    {
        return !apiRoot.empty() || !assetsRoot.empty();
    }
};

class BaseRequest
{
public:
    explicit BaseRequest(HttpMethod requestType, SubdomainType subDomainType, RequestPriority priority, const std::string &name,
                         std::map<std::string, std::string> extraParams,
                         RequestFinishedCallback callback);
    virtual ~BaseRequest() {};

    void setApiOverrideSettings(const ApiOverrideSettings &apiOverrideSettings);
    bool isApiDomainOverriden() const;

    virtual std::string url(const std::string &domain) const;

    std::string contentTypeHeader() const { return contentTypeHeader_; }
    void setContentTypeHeader(const std::string &data) { contentTypeHeader_ = data; }

    void setBearerToken(const std::string &bearerToken) { bearerToken_ = bearerToken; }
    std::string bearerToken() const { return bearerToken_; }

    void setIgnoreJsonParse() { isIgnoreJsonParse_ = true; }

    virtual std::string postData() const;
    std::string name() const { return name_; }

    virtual void handle(const std::string &arr);

    bool isCanceled();
    void callCallback();

    HttpMethod requestType() const { return requestType_; }

    void setTimeout(int ms) { timeout_ = ms; }
    int timeout() const { return timeout_; }

    RequestPriority priority() const { return priority_; }

    bool isUseDnsCache() const { return isUseDnsCache_; }
    void setUseDnsCache(bool isUse) { isUseDnsCache_ = isUse; }

    bool isWriteToLog() const { return isWriteToLog_; }
    void setNotWriteToLog() { isWriteToLog_ = false; }

    void setRetCode(ApiRetCode retCode) { retCode_ = retCode; }
    ApiRetCode retCode() const { return retCode_; }

protected:
    int timeout_ = kApiTimeout;
    HttpMethod requestType_;
    SubdomainType subDomainType_;
    std::string domainOverride_;
    RequestPriority priority_;
    bool isUseDnsCache_ = true;
    std::string name_;
    std::map<std::string, std::string> extraParams_;
    RequestFinishedCallback callback_;
    bool isWriteToLog_ = true;
    ApiRetCode retCode_ = ApiRetCode::kSuccess;
    std::string contentTypeHeader_;
    std::string bearerToken_ = "0";
    bool isIgnoreJsonParse_ = false;
    std::string json_;

    virtual std::string hostname(const std::string &domain, SubdomainType subdomain) const;
};

} // namespace wsnet
