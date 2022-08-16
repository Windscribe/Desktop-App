#ifndef TYPES_ROBERTFILTER_H
#define TYPES_ROBERTFILTER_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "node.h"

namespace types {

struct RobertFilter
{
    QString id;
    QString title;
    QString description;
    int status = 0;

    bool initFromJson(const QJsonObject &json);
    bool operator==(const RobertFilter &other) const;
    bool operator!=(const RobertFilter &other) const;

    friend QDataStream& operator <<(QDataStream &stream, const RobertFilter &o);
    friend QDataStream& operator >>(QDataStream &stream, RobertFilter &o);

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} //namespace types

#endif // TYPES_ROBERTFILTER_H