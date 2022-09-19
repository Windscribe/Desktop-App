#include <QObject>
#include <QUrlQuery>
#include "types/enums.h"

#pragma once

namespace server_api {

enum class RequestType { kGet, kPost, kDelete, kPut };

class BaseRequest : public QObject
{
    Q_OBJECT
public:
    BaseRequest(QObject *parent, RequestType requestType);
    explicit BaseRequest(QObject *parent, RequestType requestType, bool isUseFailover, int timeout);

    QUrlQuery urlQuery() const;
    RequestType requestType() const;
    void setRetCode(SERVER_API_RET_CODE retCode);
    SERVER_API_RET_CODE retCode() const;

    virtual QString contentTypeHeader() const;
    virtual QByteArray postData() const;
    virtual QString name() const = 0;
    virtual void handle(const QByteArray &arr) = 0;
    virtual void addQueryItems(QUrlQuery &urlQuery) const = 0;
    virtual QString urlPath() const = 0;

signals:
    void finished();

protected:
    void addPlatformQueryItems(QUrlQuery &urlQuery) const;
    void addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash = QString()) const;

private:
    mutable QUrlQuery urlQuery_;
    bool bUseFailover_ = true;     // Use the failover algorithm for this request
    int timeout_ = 10000;          // timeout 10 sec by default
    RequestType requestType_;
    SERVER_API_RET_CODE retCode_ = SERVER_RETURN_SUCCESS;

};

} // namespace server_api {

