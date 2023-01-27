#include "multipleaccountdetection_mac.h"
#include <QSettings>
#include <QDataStream>
#include "utils/logger.h"

// the name does not make sense for crypt
const QString MultipleAccountDetection_mac::entryName_ = "locationData2";

MultipleAccountDetection_mac::MultipleAccountDetection_mac() : crypt_(0xFA7234AAF37A31BE)
{
}

void MultipleAccountDetection_mac::userBecomeExpired(const QString &username)
{
    qCDebug(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username;
    MultipleAccountDetection_mac::TEntry entry;

    bool bNeedWrite = false;

    if (!readEntry(entry))
    {
        bNeedWrite = true;
    }

    if (bNeedWrite)
    {
        entry.username_ = username;
        entry.date_ = QDate::currentDate();
        writeEntry(entry);
        qCDebug(LOG_BASIC) << "Abuse detection: User session expired, created/updated entry in settings";
    }
}

bool MultipleAccountDetection_mac::entryIsPresent(QString &username)
{
    QSettings settings;
    bool bExists = settings.contains(entryName_);
    if (bExists)
    {
        TEntry entry;
        readEntry(entry);
        username = entry.username_;
    }
    return bExists;
}

void MultipleAccountDetection_mac::removeEntry()
{
    QSettings settings;
    settings.remove(entryName_);
    qCDebug(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

bool MultipleAccountDetection_mac::readEntry(MultipleAccountDetection_mac::TEntry &entry)
{
    QSettings settings;
    if (settings.contains(entryName_))
    {
        QByteArray buf = crypt_.decryptToByteArray(settings.value(entryName_).toByteArray());
        if (buf.isEmpty())
        {
            return false;
        }

        QDataStream stream(&buf, QIODeviceBase::ReadOnly);
        stream >> entry.username_;
        stream >> entry.date_;
        return true;
    }
    else
    {
        return false;
    }
}

void MultipleAccountDetection_mac::writeEntry(const MultipleAccountDetection_mac::TEntry &entry)
{
    QByteArray buf;
    {
        QDataStream stream(&buf, QIODeviceBase::WriteOnly);
        stream << entry.username_;
        stream << entry.date_;
    }

    QSettings settings;
    settings.setValue(entryName_, crypt_.encryptToByteArray(buf));
}
