#pragma once

#include <QJsonObject>
#include <QMap>
#include <QSettings>
#include "locationitem.h"

namespace gui_locations {

class RenamedLocationsStorage : public QObject
{
    Q_OBJECT
public:
    static constexpr int kInvalidCityId = -1;

    void setName(int locationId, int cityId, const QString &name);
    QString name(int locationId, int cityId) const;

    void setNickname(int locationId, int cityId, const QString &nickname);
    QString nickname(int locationId, int cityId) const;

    void prune(const QVector<LocationItem *> &locations);
    void reset();

    void writeToSettings();
    void readFromSettings();

    bool operator== (const RenamedLocationsStorage &other) const {
        return (names_ == other.names_ && nicknames_ == other.nicknames_);
    }

private:
    QMap<QPair<int, int>, QString> names_;
    QMap<QPair<int, int>, QString> nicknames_;

    static constexpr quint32 magic_ = 0xAA43C2AE;
    static constexpr int versionForSerialization_ = 1;
};

} // namespace gui_locations