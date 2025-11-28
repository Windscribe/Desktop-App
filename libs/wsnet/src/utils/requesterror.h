#pragma once
#include "WSNetRequestError.h"
#include <memory>
#include <optional>

namespace wsnet {

enum class RequestErrorType { kCurl, kCares };

// A container for an HTTP error, which may contain either a curl error or cares
class RequestError : public WSNetRequestError
{
public:
    explicit RequestError(int errorCode, RequestErrorType type);
    explicit RequestError(int errorCode, RequestErrorType type, long osErrorCode);

    bool isSuccess() const override;
    bool isDnsError() const override;
    bool isCurlError() const override;
    std::string toString() const override;
    bool isNoNetworkError() const override;
    int httpResponseCode() const override;
    void setHttpResponseCode(int httpResponseCode);

    static std::shared_ptr<WSNetRequestError> createCaresSuccess();
    static std::shared_ptr<WSNetRequestError> createCaresTimeout();

private:
    int errorCode_;
    RequestErrorType type_;
    int httpResponseCode_;

    // errno number from last connect failure
    // the number is OS and system specific
    // only makes sense for a curl error
    std::optional<long> osErrorCode_;
};

} // namespace wsnet
