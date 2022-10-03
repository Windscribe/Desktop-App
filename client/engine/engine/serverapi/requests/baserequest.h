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

    RequestType requestType() const;
    int timeout() const;

    bool isWriteToLog() const { return isWriteToLog_; }
    void setNotWriteToLog() { isWriteToLog_ = false; }

    void setRetCode(SERVER_API_RET_CODE retCode);
    SERVER_API_RET_CODE retCode() const;

signals:
    void finished();

protected:
    QString hostname(const QString &domain, SudomainType subdomain) const;
    void addPlatformQueryItems(QUrlQuery &urlQuery) const;
    void addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash = QString()) const;

private:
    bool bUseFailover_ = true;     // Use the failover algorithm for this request
    int timeout_ = 5000;          // timeout 5 sec by default
    RequestType requestType_;
    bool isWriteToLog_ = true;
    SERVER_API_RET_CODE retCode_ = SERVER_RETURN_SUCCESS;
};

} // namespace server_api

