#pragma once

#include <QString>
#include <QJsonObject>

namespace api_responses {

struct CaptchaData {
    QString background;
    QString slider;
    int top;
};


class AuthToken
{
public:
    AuthToken(const std::string &json);

    bool isValid() const { return isValid_; }
    QString token() const { return token_; }
    bool isCaptchaRequired() const { return isCaptchaRequired_; }
    CaptchaData captchaData() const { return captchaData_; }
    bool isRateLimitError() const { return isRateLimitError_; }

private:
    bool isValid_ = false;
    bool isRateLimitError_ = false;
    QString token_;
    bool isCaptchaRequired_;
    CaptchaData captchaData_;

};

} // api_responses namespace
