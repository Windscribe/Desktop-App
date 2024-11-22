#include "multipleaccountdetection_posix.h"
#include <QSettings>
#include <QDataStream>
#include <QIODevice>
#include "utils/log/categories.h"

// the name does not make sense for crypt
const QString MultipleAccountDetection_posix::entryName_ = "locationData2";

MultipleAccountDetection_posix::MultipleAccountDetection_posix() : crypt_(0xFA7234AAF37A31BE)
{
}

void MultipleAccountDetection_posix::userBecomeExpired(const QString &username)
{
    qCDebug(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username;
    MultipleAccountDetection_posix::TEntry entry;

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

bool MultipleAccountDetection_posix::entryIsPresent(QString &username)
{
    QSettings settings("Windscribe", "location");
    bool bExists = settings.contains(entryName_);
    if (bExists)
    {
        TEntry entry;
        readEntry(entry);
        username = entry.username_;
    }
    return bExists;
}

void MultipleAccountDetection_posix::removeEntry()
{
    QSettings settings("Windscribe", "location");
    settings.remove(entryName_);
    qCDebug(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

bool MultipleAccountDetection_posix::readEntry(MultipleAccountDetection_posix::TEntry &entry)
{
    QSettings settings("Windscribe", "location");
    if (settings.contains(entryName_))
    {
        QByteArray buf = crypt_.decryptToByteArray(settings.value(entryName_).toByteArray());
        if (buf.isEmpty())
        {
            return false;
        }

        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> entry.username_;
        stream >> entry.date_;
        return true;
    }
    else
    {
        return false;
    }
}

void MultipleAccountDetection_posix::writeEntry(const MultipleAccountDetection_posix::TEntry &entry)
{
    QByteArray buf;
    {
        QDataStream stream(&buf, QIODevice::WriteOnly);
        stream << entry.username_;
        stream << entry.date_;
    }

    QSettings settings("Windscribe", "location");
    settings.setValue(entryName_, crypt_.encryptToByteArray(buf));
}
