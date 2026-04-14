#pragma once

#include <memory>
#include <QString>
#include "location.h"

namespace wsnet { class WSNetServerLocations; }

namespace api_responses {

class ServerList
{
public:
    explicit ServerList(std::shared_ptr<wsnet::WSNetServerLocations> serverLocations);

    QVector<Location> locations() const { return locations_; }

private:
    QVector<Location> locations_;
};

} //namespace api_responses
