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
    // Country-level entries (cityId == kInvalidCityId) are valid for names_ but not for
    // nicknames_ — product has no UI for country nicknames.
    QSet<QPair<int, int>> allowedNames;
    QSet<QPair<int, int>> allowedNicknames;
    for (const auto location : locations) {
        allowedNames.insert(QPair<int, int>(location->location().idNum, kInvalidCityId));
        for (const auto &city : location->location().cities) {
            QPair<int, int> p(location->location().idNum, city.idNum);
            allowedNames.insert(p);
            allowedNicknames.insert(p);
        }
    }

    for (const auto &pair : names_.keys()) {
        if (!allowedNames.contains(pair)) {
            qCDebug(LOG_LOCATION_LIST) << "Removing renamed city" << pair.first << pair.second << names_[pair];
            names_.remove(pair);
        }
    }
    for (const auto &pair : nicknames_.keys()) {
        if (!allowedNicknames.contains(pair)) {
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
            } else {
                // Drop country-level nickname entries. Product has no UI for country nicknames;
                // any such entries are poison from the pre-fix bug where setData wrote to
                // (garbage, kInvalidCityId) keyed on an uninitialized best-location idNum.
                bool migrated = false;
                for (const auto &pair : nicknames_.keys()) {
                    if (pair.second == kInvalidCityId) {
                        nicknames_.remove(pair);
                        migrated = true;
                    }
                }
                if (migrated) {
                    writeToSettings();
                }
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
