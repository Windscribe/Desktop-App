#pragma once

#include <memory>
#include <QSet>
#include <QString>
#include "location.h"

namespace wsnet { class WSNetServerLocations; }

namespace api_responses {

class ServerList
{
public:
    explicit ServerList(std::shared_ptr<wsnet::WSNetServerLocations> serverLocations);

    QVector<Location> locations() const { return locations_; }
    // Ids of the nodes the server list marks force-disconnect (fd=1). They are absent from
    // locations() -- Group::initFromWsnet leaves them out -- so this is the only thing that names
    // them, and the only way a live connection can tell it is sitting on one.
    QSet<int> forceDisconnectNodeIds() const { return forceDisconnectNodeIds_; }

private:
    QVector<Location> locations_;
    QSet<int> forceDisconnectNodeIds_;
};

} //namespace api_responses
