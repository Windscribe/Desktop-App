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
        QByteArray arr;
        {
            QDataStream ds(&arr, QIODevice::WriteOnly);
            ds << magic_ << versionForSerialization_ << id_;
        }
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
        QDataStream ds(&arr, QIODevice::ReadOnly);
        quint32 magic, version;
        ds >> magic;
        if (magic != magic_)
        {
            return;
        }
        ds >> version;
        if (version > versionForSerialization_)
        {
            return;
        }
        ds >> id_;
        isValid_ = true;
    }
}

} //namespace locationsmodel
