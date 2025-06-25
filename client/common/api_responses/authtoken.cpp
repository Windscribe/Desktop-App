#include "authtoken.h"
#include <QJsonDocument>
#include "utils/log/categories.h"

const int typeIdAuthToken = qRegisterMetaType<api_responses::AuthToken>("api_responses::AuthToken");

namespace api_responses {

AuthToken::AuthToken(const std::string &json)
{
    QJsonParseError errCode;
    auto doc = QJsonDocument::fromJson(QByteArray(json.c_str()), &errCode);
    if (errCode.error != QJsonParseError::NoError) {
        return;
    }

    auto jsonObject = doc.object();
    if (jsonObject.contains("errorCode")) {
        int errorCode = jsonObject["errorCode"].toInt();
        // We have been rate limited by the server
        if (errorCode == 707) {
            isRateLimitError_ = true;
            isValid_ = true;
        } else {
            // For now only error 707 is possible, anything else is an error that requires attention
            qCWarning(LOG_BASIC) << "Unknown error code in AuthToken:" << errorCode;
            return;
        }
    }

    if (!jsonObject.contains("data")) {
        return;
    }

    auto jsonData =  jsonObject["data"].toObject();
    token_ = jsonData["token"].toString();
    if (token_.isEmpty()) {
        return;
    }

    if (jsonData.contains("captcha")) {
        isCaptchaRequired_ = true;
        auto jsonCaptcha = jsonData["captcha"].toObject();
        if (jsonCaptcha.contains("ascii_art")) {
            captchaData_.isAsciiCaptcha = true;
            captchaData_.asciiArt = jsonCaptcha["ascii_art"].toString();
        } else {
            captchaData_.isAsciiCaptcha = false;
            captchaData_.background = jsonCaptcha["background"].toString();
            captchaData_.slider = jsonCaptcha["slider"].toString();
            captchaData_.top = jsonCaptcha["top"].toInteger();
        }

    } else {
        isCaptchaRequired_ = false;
    }

    isValid_ = true;
}

} // api_responses namespace
