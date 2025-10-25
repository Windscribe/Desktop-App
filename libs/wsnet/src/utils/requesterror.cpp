#include "requesterror.h"
#include <curl/curl.h>
#include <ares.h>

namespace wsnet {

RequestError::RequestError(int errorCode, RequestErrorType type) :
    errorCode_(errorCode), type_(type), httpResponseCode_(0)
{
}

RequestError::RequestError(int errorCode, RequestErrorType type, long osErrorCode) :
    errorCode_(errorCode), type_(type), httpResponseCode_(0), osErrorCode_(osErrorCode)
{
}


bool RequestError::isSuccess() const
{
    if (type_ == RequestErrorType::kCurl)
        return errorCode_ == CURLE_OK;
    else
        return errorCode_ == ARES_SUCCESS;
}

bool RequestError::isDnsError() const
{
    return type_ == RequestErrorType::kCares && errorCode_ != ARES_SUCCESS;
}

bool RequestError::isCurlError() const
{
    return type_ == RequestErrorType::kCurl  && errorCode_ != CURLE_OK;
}

std::string RequestError::toString() const
{
    if (type_ == RequestErrorType::kCurl) {
        if (osErrorCode_.has_value())
            return std::string("curl: ") + curl_easy_strerror((CURLcode)errorCode_) + "; os_errno = " + std::to_string(osErrorCode_.value());
        else
            return std::string("curl: ") + curl_easy_strerror((CURLcode)errorCode_);
    } else {
        return std::string("cares: ") + ares_strerror(errorCode_);
    }
}

bool RequestError::isNoNetworkError() const
{
    // Error codes, including OS-specific ones, based on which we can determine the lack of connectivity
    if (type_ == RequestErrorType::kCurl) {
        if (osErrorCode_.has_value()) {

#ifdef _WIN32
            if (osErrorCode_.value() == WSAEHOSTUNREACH || osErrorCode_.value() == WSAENETUNREACH || osErrorCode_.value() == WSAENETDOWN) {
                return true;
            }
// Mac, Linux
#else
            if (osErrorCode_.value() == EHOSTUNREACH || osErrorCode_.value() == ENETUNREACH || osErrorCode_.value() == ENETDOWN ) {
                return true;
            }
#endif
        }
    } else {    // c-ares
        return errorCode_ == ARES_ECONNREFUSED;
    }

    return false;
}

int RequestError::httpResponseCode() const
{
    return httpResponseCode_;
}

void RequestError::setHttpResponseCode(int httpResponseCode)
{
    httpResponseCode_ = httpResponseCode;
}

std::shared_ptr<WSNetRequestError> RequestError::createCaresSuccess()
{
    return std::make_shared<RequestError>(ARES_SUCCESS, RequestErrorType::kCares);
}

std::shared_ptr<WSNetRequestError> RequestError::createCaresTimeout()
{
    return std::make_shared<RequestError>(ARES_ETIMEOUT, RequestErrorType::kCurl);
}


} // namespace wsnet

