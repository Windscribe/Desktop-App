#pragma once

#include <QString>
#include "location.h"

namespace api_responses {

class ServerList
{
public:
    ServerList(const std::string &json);

    QVector<Location> locations() const { return locations_; }
    QStringList forceDisconnectNodes() const { return forceDisconnectNodes_; }
    QString countryOverride() const { return countryOverride_; }

private:
    QVector<Location> locations_;
    QStringList forceDisconnectNodes_;
    QString countryOverride_;

};

} //namespace api_responses
