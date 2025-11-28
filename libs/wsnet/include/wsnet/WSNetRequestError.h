#pragma once

#include <cstdint>
#include <string>
#include "scapix_object.h"

namespace wsnet {

// A container for a request error, which may contain either a curl error or cares
class WSNetRequestError : public scapix_object<WSNetRequestError>
{
public:
    virtual ~WSNetRequestError() {}

    virtual bool isSuccess() const = 0;
    virtual bool isDnsError() const = 0;
    virtual bool isCurlError() const = 0;
    virtual std::string toString() const = 0;

    // Based on some curl and cares error codes we can assume that the error is caused by the lack of network connectivty
    // We need to separate these errors so that we don't have to switch failovers for these types of errors
    virtual bool isNoNetworkError() const = 0;

    virtual int httpResponseCode() const = 0;
};

} // namespace wsnet
