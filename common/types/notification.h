#ifndef TYPES_NOTIFICATION_H
#define TYPES_NOTIFICATION_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "node.h"

namespace types {

struct Notification
{
    qint64 id = 0;
    QString title;
    QString message;
    qint64 date = 0;
    int permFree = 0;
    int permPro = 0;
    int popup = 0;

    bool initFromJson(const QJsonObject &json);
    bool operator==(const Notification &other) const;
    bool operator!=(const Notification &other) const;

    friend QDataStream& operator <<(QDataStream &stream, const Notification &o);
    friend QDataStream& operator >>(QDataStream &stream, Notification &o);

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} //namespace types

#endif // TYPES_NOTIFICATION_H
