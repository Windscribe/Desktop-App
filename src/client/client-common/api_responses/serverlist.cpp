#include "serverlist.h"
#include <wsnet/WSNetServerLocations.h>

namespace api_responses {

ServerList::ServerList(std::shared_ptr<wsnet::WSNetServerLocations> serverLocations)
{
    if (!serverLocations)
        return;

    for (const auto &loc : serverLocations->locations()) {
        Location location;
        location.initFromWsnet(loc);
        locations_ << location;
    }
}

} // namespace api_responses
