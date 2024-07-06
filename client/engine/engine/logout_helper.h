#pragma once
#include <QString>
#include <wsnet/WSNet.h>
#include "utils/logger.h"

class LogoutHelper final
{
public:
    ~LogoutHelper()
    {
        if (request_) {
            qCDebug(LOG_BASIC) << "deleteSession request canceled";
            request_->cancel();
        }
    }

    void logout(const QString &authHash)
    {
        auto callback = [this](wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData)
        {
            request_.reset();
            // no result needed
            qCDebug(LOG_BASIC) << "deleteSession request finished with serverApiRetCode:" << (int)serverApiRetCode;
        };
        request_ = wsnet::WSNet::instance()->serverAPI()->deleteSession(authHash.toStdString(), callback);
    }

private:
    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;
};
