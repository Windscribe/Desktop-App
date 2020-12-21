#include "bestlocation.h"

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
        ProtoApiInfo::BestLocation b;

        *b.mutable_location_id() = id_.toProtobuf();

        size_t size = b.ByteSizeLong();
        QByteArray arr(size, Qt::Uninitialized);
        b.SerializeToArray(arr.data(), size);

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
        ProtoApiInfo::BestLocation b;
        if (b.ParseFromArray(arr.data(), arr.size()))
        {
            isValid_ = true;
            id_ = LocationID::createFromProtoBuf(b.location_id());
        }
    }
}

} //namespace locationsmodel
