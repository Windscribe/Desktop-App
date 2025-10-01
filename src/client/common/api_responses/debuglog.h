#pragma once

#include <string>

namespace api_responses {

class DebugLog
{
public:
    DebugLog(const std::string &json);

    bool isSuccess() const { return bSuccess_; }

private:
    bool bSuccess_;

};

} //namespace api_responses
