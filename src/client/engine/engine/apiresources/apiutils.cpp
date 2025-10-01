#include "apiutils.h"

void ApiUtils::mergeWindflixLocations(QVector<api_responses::Location> &locations)
{
    // Build a new list of server locations to merge, removing them from the old list.
    // Currently we merge all WindFlix locations into the corresponding global locations.
    QVector<api_responses::Location> locationsToMerge;
    QMutableVectorIterator<api_responses::Location> it(locations);
    while (it.hasNext()) {
        api_responses::Location &location = it.next();
        if (location.getName().startsWith("WINDFLIX")) {
            locationsToMerge.append(location);
            it.remove();
        }
    }
    if (!locationsToMerge.size())
        return;

    // Map city names to locations for faster lookups.
    QHash<QString, api_responses::Location *> location_hash;
    for (auto &location: locations) {
        for (int i = 0; i < location.groupsCount(); ++i)
        {
            const api_responses::Group group = location.getGroup(i);
            location_hash.insert(location.getCountryCode() + group.getCity(), &location);
        }
    }

    // Merge the locations.
    QMutableVectorIterator<api_responses::Location> itm(locationsToMerge);
    while (itm.hasNext()) {
        api_responses::Location &location = itm.next();
        const auto country_code = location.getCountryCode();

        for (int i = 0; i < location.groupsCount(); ++i)
        {
            api_responses::Group group = location.getGroup(i);
            group.setOverrideDnsHostName(location.getDnsHostName());

            auto target = location_hash.find(country_code + group.getCity());
            WS_ASSERT(target != location_hash.end());
            if (target != location_hash.end())
            {
                target.value()->addGroup(group);
            }
        }
        itm.remove();
    }

}
