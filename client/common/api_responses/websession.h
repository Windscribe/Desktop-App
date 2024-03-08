#pragma once

#include <QString>

namespace api_responses {

class WebSession
{
public:
    WebSession(const std::string &json);

    QString token() const { return token_; }

private:
    QString token_;

};

} //namespace api_responses
