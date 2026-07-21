#pragma once

#include <QString>

namespace api_responses {

class ServerCredentials
{
public:
    ServerCredentials() {}
    ServerCredentials(const std::string &json);
    ServerCredentials(const QString &username, const QString &password) : username_(username), password_(password) {}

    QString username() const { return username_; }
    QString password() const { return password_; }

private:
    QString username_;
    QString password_;

};

} //namespace api_responses
