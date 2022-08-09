#include "bestlocation.h"

#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

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
            ds << magic_;
            ds << versionForSerialization_;
            ds << id_;
        }

        QSettings settings;
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        settings.setValue("bestLocation", simpleCrypt.encryptToString(arr));
    }
}

void BestLocation::loadFromSettings()
{
    QSettings settings;
    if (settings.contains("bestLocation"))
    {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value("bestLocation", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> id_;
                if (ds.status() == QDataStream::Ok)
                {
                    isValid_ = true;
                }
            }
        }
    }
}

} //namespace locationsmodel
