#pragma once

#include <QString>

namespace api_responses {

class WgConfigsConnect
{
public:
    WgConfigsConnect(const std::string &json);

    bool isErrorCode() const { return isErrorCode_; }
    int errorCode() const { return errorCode_; }
    QString ipAddress() const { return ipAddress_; }
    QString dnsAddress() const { return dnsAddress_; }

private:
    bool isErrorCode_ = false;
    int errorCode_ = 0;
    QString ipAddress_;
    QString dnsAddress_;
};

} //namespace api_responses
