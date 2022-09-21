#pragma once

#include <QObject>
#include <QUrlQuery>
#include "types/enums.h"


namespace server_api {

enum class RequestType { kGet, kPost, kDelete, kPut };

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
    void setRetCode(SERVER_API_RET_CODE retCode);
    SERVER_API_RET_CODE retCode() const;

signals:
    void finished();

protected:
    void addPlatformQueryItems(QUrlQuery &urlQuery) const;
    void addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash = QString()) const;

    QString hostname_;

private:
    bool bUseFailover_ = true;     // Use the failover algorithm for this request
    int timeout_ = 10000;          // timeout 10 sec by default
    RequestType requestType_;
    SERVER_API_RET_CODE retCode_ = SERVER_RETURN_SUCCESS;
};

} // namespace server_api

