#pragma once

#include <map>
#include "WSNetHttpRequest.h"
#include "WSNetServerAPI.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

enum class SubdomainType { kApi, kAssets, kTunnelTest };
enum class RequestPriority { kNormal, kHigh };

using RequestFinishedCallback = std::shared_ptr<CancelableCallback<WSNetRequestFinishedCallback>>;

class BaseRequest
{
public:
    explicit BaseRequest(HttpMethod requestType, SubdomainType subDomainType, RequestPriority priority, const std::string &name,
                         std::map<std::string, std::string> extraParams,
                         RequestFinishedCallback callback);
    virtual ~BaseRequest() {};

    virtual std::string url(const std::string &domain) const;

    std::string contentTypeHeader() const { return contentTypeHeader_; }
    void setContentTypeHeader(const std::string &data) { contentTypeHeader_ = data; }

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

    void setRetCode(ServerApiRetCode retCode) { retCode_ = retCode; }
    ServerApiRetCode retCode() const { return retCode_; }

protected:
    int timeout_ = 5000;           // timeout 5 sec by default
    HttpMethod requestType_;
    SubdomainType subDomainType_;
    RequestPriority priority_;
    bool isUseDnsCache_ = true;
    std::string name_;
    std::map<std::string, std::string> extraParams_;
    RequestFinishedCallback callback_;
    bool isWriteToLog_ = true;
    ServerApiRetCode retCode_ = ServerApiRetCode::kSuccess;
    std::string contentTypeHeader_;
    bool isIgnoreJsonParse_ = false;
    std::string json_;

    std::string hostname(const std::string &domain, SubdomainType subdomain) const;
};

} // namespace wsnet

