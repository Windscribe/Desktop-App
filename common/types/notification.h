#ifndef TYPES_NOTIFICATION_H
#define TYPES_NOTIFICATION_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "node.h"

namespace types {

struct Notification
{
public:
    Notification() :
        id(0),
        date(0),
        permFree(0),
        permPro(0),
        popup(0)
    {}

    bool initFromJson(const QJsonObject &json);
    bool operator==(const Notification &other) const;
    bool operator!=(const Notification &other) const;

    qint64 id;
    QString title;
    QString message;
    qint64 date;
    int permFree;
    int permPro;
    int popup;
};

} //namespace types

#endif // TYPES_NOTIFICATION_H
