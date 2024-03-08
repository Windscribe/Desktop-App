#pragma once

#include "WSNetHttpNetworkManager.h"
#include "failoverdata.h"

namespace wsnet {

typedef std::function<void(const std::vector<FailoverData> &data)> FailoverCallback;

class BaseFailover
{
public:
    explicit BaseFailover(const std::string &uniqueId, WSNetHttpNetworkManager *httpNetworkManager = nullptr) :
        uniqueId_(uniqueId), httpNetworkManager_(httpNetworkManager)
    {
    }
    virtual ~BaseFailover() {}

    // Return false, if executed asynchronously. In case of asynchronous execution, callback will be called
    // if data is empty then it is implied that failed
    virtual bool getData(bool bIgnoreSslErrors, std::vector<FailoverData> &data, FailoverCallback callback) = 0;
    virtual std::string name() const = 0;
    std::string uniqueId() const { return uniqueId_; }

protected:
    WSNetHttpNetworkManager *httpNetworkManager_;
    std::string uniqueId_;
};


} // namespace wsnet

