#pragma once

#include <QString>
#include <QStringList>

namespace api_responses {

class WgConfigsInit
{
public:
    WgConfigsInit(const std::string &json);

    bool isErrorCode() const { return isErrorCode_; }
    int errorCode() const { return errorCode_; }
    QString presharedKey() const { return presharedKey_; }
    QString allowedIps() const { return allowedIps_; }
    QString allowedIpsV6() const { return allowedIpsV6_; }
    QStringList hashedCIDR() const { return hashedCIDR_; }
    QStringList hashedCIDRv6() const { return hashedCIDRv6_; }

private:
    bool isErrorCode_ = false;
    int errorCode_ = 0;
    QString presharedKey_;
    QString allowedIps_;
    QString allowedIpsV6_;
    QStringList hashedCIDR_;
    QStringList hashedCIDRv6_;
};

} //namespace api_responses
