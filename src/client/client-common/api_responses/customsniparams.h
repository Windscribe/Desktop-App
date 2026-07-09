#pragma once

#include <QString>

namespace api_responses {

class CustomSniParams
{
public:
    CustomSniParams() = default;
    explicit CustomSniParams(const std::string &json);

    bool isValid() const { return !domain_.isEmpty(); }
    QString domain() const { return domain_; }

private:
    QString domain_;
};

} //namespace api_responses
