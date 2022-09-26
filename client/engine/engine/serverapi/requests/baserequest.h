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
    BaseRequest(QObject *parent, RequestType requestType, const QString &hostname);
    explicit BaseRequest(QObject *parent, RequestType requestType, const QString &hostname, bool isUseFailover, int timeout);

    virtual QUrl url() const = 0;
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
    QString hostname(SudomainType subdomain) const;
    void addPlatformQueryItems(QUrlQuery &urlQuery) const;
    void addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash = QString()) const;

private:
    bool bUseFailover_ = true;     // Use the failover algorithm for this request
    int timeout_ = 10000;          // timeout 10 sec by default
    RequestType requestType_;
    bool isWriteToLog_ = true;
    SERVER_API_RET_CODE retCode_ = SERVER_RETURN_SUCCESS;
    QString hostname_;
};

} // namespace server_api

