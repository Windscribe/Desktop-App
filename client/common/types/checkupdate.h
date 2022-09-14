#ifndef TYPES_CHECKUPDATE_H
#define TYPES_CHECKUPDATE_H

#include <QString>
#include <QJsonObject>
#include "enums.h"

namespace types {

struct CheckUpdate
{
    bool isAvailable = false;
    QString version;
    UPDATE_CHANNEL updateChannel = UPDATE_CHANNEL_RELEASE;
    int latestBuild = 0;
    QString url;
    bool isSupported = false;
    QString sha256;

    bool operator==(const CheckUpdate &other) const;
    bool operator!=(const CheckUpdate &other) const;
    static CheckUpdate createFromApiJson(QJsonObject &json, bool &outSuccess, QString &outErrorMessage);
};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
