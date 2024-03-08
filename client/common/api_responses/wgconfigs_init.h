#pragma once

#include <QString>

namespace api_responses {

class WgConfigsInit
{
public:
    WgConfigsInit(const std::string &json);

    bool isErrorCode() const { return isErrorCode_; }
    int errorCode() const { return errorCode_; }
    QString presharedKey() const { return presharedKey_; }
    QString allowedIps() const { return allowedIps_; }

private:
    bool isErrorCode_ = false;
    int errorCode_ = 0;
    QString presharedKey_;
    QString allowedIps_;
};

} //namespace api_responses
