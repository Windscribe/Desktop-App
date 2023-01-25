#pragma once

#include <QObject>
#include <QUrlQuery>
#include "types/enums.h"

namespace server_api {

enum class RequestType { kGet, kPost, kDelete, kPut };
enum class SudomainType { kApi, kAssets, kTunnelTest };

class BaseRequest : public QObject
{
    Q_OBJECT
public:
    BaseRequest(QObject *parent, RequestType requestType);
    explicit BaseRequest(QObject *parent, RequestType requestType, bool isUseFailover, int timeout);

    virtual QUrl url(const QString &domain) const = 0;
    virtual QString contentTypeHeader() const;
    virtual QByteArray postData() const;
    virtual QString name() const = 0;
    virtual void handle(const QByteArray &arr) = 0;

    RequestType requestType() const { return requestType_; }
    int timeout() const { return timeout_; }

    bool isWriteToLog() const { return isWriteToLog_; }
    void setNotWriteToLog() { isWriteToLog_ = false; }

    void setNetworkRetCode(SERVER_API_RET_CODE retCode) { networkRetCode_ = retCode; }
    SERVER_API_RET_CODE networkRetCode() const { return networkRetCode_; }

    void setCancelled(bool cancelled);
    bool isCancelled() const;

signals:
    void finished();

protected:
    QString hostname(const QString &domain, SudomainType subdomain) const;

private:
    bool bUseFailover_ = true;     // Use the failover algorithm for this request
    int timeout_ = 5000;          // timeout 5 sec by default
    RequestType requestType_;
    bool isWriteToLog_ = true;
    SERVER_API_RET_CODE networkRetCode_ = SERVER_RETURN_SUCCESS;
    bool cancelled_ = false;
};

} // namespace server_api

