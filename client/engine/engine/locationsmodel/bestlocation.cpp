#include "bestlocation.h"

#include <QIODevice>
#include <QSettings>

namespace locationsmodel {

BestLocation::BestLocation() : isValid_(false), isDetectedFromThisAppStart_(false), isDetectedWithDisconnectedIps_(0)
{
    loadFromSettings();
}

BestLocation::~BestLocation()
{
    saveToSettings();
}

bool BestLocation::isValid() const
{
    return isValid_;
}

LocationID BestLocation::getId() const
{
    Q_ASSERT(isValid_);
    return id_;
}

void BestLocation::set(const LocationID &id, bool isDetectedFromThisAppStart, bool isDetectedWithDisconnectedIps)
{
    isValid_ = true;
    isDetectedFromThisAppStart_ = isDetectedFromThisAppStart;
    isDetectedWithDisconnectedIps_ = isDetectedWithDisconnectedIps;
    id_ = id;
}

bool BestLocation::isDetectedFromThisAppStart() const
{
    Q_ASSERT(isValid_);
    return isDetectedFromThisAppStart_;
}

bool BestLocation::isDetectedWithDisconnectedIps() const
{
    Q_ASSERT(isValid_);
    return isDetectedWithDisconnectedIps_;
}

void BestLocation::saveToSettings()
{
    if (isValid_)
    {
        QJsonObject json;
        json["version"] = versionForSerialization_;
        json["location"] = id_.toJsonObject();
        QCborValue cbor = QCborValue::fromJsonValue(json);
        QByteArray arr = cbor.toCbor();
        QSettings settings;
        settings.setValue("bestLocation", arr);
    }
}

void BestLocation::loadFromSettings()
{
    QSettings settings;
    if (settings.contains("bestLocation"))
    {
        QByteArray arr = settings.value("bestLocation").toByteArray();
        QCborValue cbor = QCborValue::fromCbor(arr);
        if (!cbor.isInvalid() && cbor.isMap())
        {
            const QJsonObject &obj = cbor.toJsonValue().toObject();
            if (obj.contains("version") && obj["version"].toInt(INT_MAX) <= versionForSerialization_)
            {
                if (id_.fromJsonObject(obj["location"].toObject()))
                {
                    isValid_ = true;
                }
            }
        }
    }
}

} //namespace locationsmodel
