#include "baserequest.h"

#pragma once

namespace server_api {

class AuthenticatedRequest : public BaseRequest
{
    Q_OBJECT
public:
    AuthenticatedRequest(QObject *parent, RequestType requestType, const QString &authHash);
    explicit AuthenticatedRequest(QObject *parent, RequestType requestType, bool isUseFailover, int timeout, const QString &authHash);

    void addQueryItems(QUrlQuery &urlQuery) const override;

private:
    QString authHash_;
};

} // namespace server_api {

