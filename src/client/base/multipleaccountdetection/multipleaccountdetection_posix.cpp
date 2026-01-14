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

void MultipleAccountDetection_posix::userBecomeExpired(const QString &username, const QString &userId)
{
    qCInfo(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username << "userId =" << userId;
    MultipleAccountDetection_posix::TEntry entry;

    if (readEntry(entry) && entry.username_ != username) {
        // Don't update entry if username is different
        return;
    }

    entry.username_ = username;
    entry.userId_ = userId;
    entry.date_ = QDate::currentDate();
    writeEntry(entry);
    qCInfo(LOG_BASIC) << "Abuse detection: User session expired, created/updated entry in settings";
}

bool MultipleAccountDetection_posix::entryIsPresent(QString &username, QString &userId)
{
    TEntry entry;
    if (readEntry(entry)) {
        username = entry.username_;
        userId = entry.userId_;
        return true;
    }
    return false;
}

void MultipleAccountDetection_posix::removeEntry()
{
    QSettings settings("Windscribe", "location");
    settings.remove(entryName_);
    qCInfo(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

bool MultipleAccountDetection_posix::readEntry(MultipleAccountDetection_posix::TEntry &entry)
{
    QSettings settings("Windscribe", "location");
    if (settings.contains(entryName_)) {
        QByteArray buf = crypt_.decryptToByteArray(settings.value(entryName_).toByteArray());
        if (buf.isEmpty()) {
            return false;
        }

        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> entry.username_;
        stream >> entry.date_;

        if (!stream.atEnd()) {
            stream >> entry.userId_;
        } else {
            entry.userId_ = QString();
        }
        return true;
    } else {
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
        stream << entry.userId_;
    }

    QSettings settings("Windscribe", "location");
    settings.setValue(entryName_, crypt_.encryptToByteArray(buf));
}
