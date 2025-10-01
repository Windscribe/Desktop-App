#include "renamedlocationsstorage.h"

#include <QIODevice>
#include "types/global_consts.h"
#include "utils/simplecrypt.h"

namespace gui_locations {

void RenamedLocationsStorage::setName(int locationId, int cityId, const QString &name)
{
    if (name.isEmpty()) {
        names_.remove(QPair<int, int>(locationId, cityId));
    } else {
        names_[QPair<int, int>(locationId, cityId)] = name;
    }
}

QString RenamedLocationsStorage::name(int locationId, int cityId) const
{
    return names_.value(QPair<int, int>(locationId, cityId), "");
}

void RenamedLocationsStorage::setNickname(int locationId, int cityId, const QString &nickname)
{
    if (nickname.isEmpty()) {
        nicknames_.remove(QPair<int, int>(locationId, cityId));
    } else {
        nicknames_[QPair<int, int>(locationId, cityId)] = nickname;
    }
}

QString RenamedLocationsStorage::nickname(int locationId, int cityId) const
{
    return nicknames_.value(QPair<int, int>(locationId, cityId), "");
}

void RenamedLocationsStorage::prune(const QVector<LocationItem *> &locations)
{
    // Build a lookup table of location id and city id pairs from current locations
    QSet<QPair<int, int>> pairs;
    for (auto location : locations) {
        pairs.insert(QPair<int, int>(location->location().idNum, kInvalidCityId));
        for (auto city : location->location().cities) {
            pairs.insert(QPair<int, int>(location->location().idNum, city.idNum));
        }
    }

    // Remove all pairs that are not in the lookup table
    for (auto pair : names_.keys()) {
        if (!pairs.contains(pair)) {
            qCDebug(LOG_LOCATION_LIST) << "Removing renamed city" << pair.first << pair.second << names_[pair];
            names_.remove(pair);
        }
    }
    for (auto pair : nicknames_.keys()) {
        if (!pairs.contains(pair)) {
            qCDebug(LOG_LOCATION_LIST) << "Removing renamed city nickname" << pair.first << pair.second << nicknames_[pair];
            nicknames_.remove(pair);
        }
    }
    writeToSettings();
}

void RenamedLocationsStorage::reset()
{
    names_.clear();
    nicknames_.clear();
    writeToSettings();
}

void RenamedLocationsStorage::readFromSettings()
{
    names_.clear();
    nicknames_.clear();

    QSettings settings;
    if (!settings.contains("renamedLocations")) {
        return;
    }

    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QString str = settings.value("renamedLocations", "").toString();
    QByteArray arr = simpleCrypt.decryptToByteArray(str);

    QDataStream ds(&arr, QIODevice::ReadOnly);

    quint32 magic, version;
    ds >> magic;
    if (magic == magic_) {
        ds >> version;
        if (version <= versionForSerialization_) {
            ds >> names_ >> nicknames_;
            if (ds.status() != QDataStream::Ok) {
                qCDebug(LOG_BASIC) << "Failed to read renamed locations from settings";
                names_.clear();
                nicknames_.clear();
            }
        }
    }
}

void RenamedLocationsStorage::writeToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << names_ << nicknames_;
    }
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("renamedLocations", simpleCrypt.encryptToString(arr));
}

} // namespace gui_locations
