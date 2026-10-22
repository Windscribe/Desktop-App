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

        for (const auto &group : loc.groups) {
            for (const auto &node : group.nodes) {
                if (node.forceDisconnect) {
                    forceDisconnectNodeIds_ << node.id;
                }
            }
        }
    }
}

} // namespace api_responses
