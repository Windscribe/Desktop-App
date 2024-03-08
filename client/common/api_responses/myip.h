#pragma once

#include <QString>

namespace api_responses {

class MyIp
{
public:
    MyIp(const std::string &json);

    QString ip() const { return ip_; }

private:
    QString ip_;

};

} //namespace api_responses
