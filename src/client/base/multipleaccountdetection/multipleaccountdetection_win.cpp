#include "multipleaccountdetection_win.h"
#include "utils/log/categories.h"
#include <QDate>

MultipleAccountDetection_win::MultipleAccountDetection_win()
{
}

void MultipleAccountDetection_win::userBecomeExpired(const QString &username, const QString &userId)
{
    qCInfo(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username << "userId =" << userId;

    SecretValue_win::TEntry entry;
    if (secretValue_.isExistValue(entry) && entry.username_ != username) {
        // Don't update entry if username is different
        return;
    }

    entry.username_ = username;
    entry.userId_ = userId;
    entry.date_ = QDate::currentDate();
    secretValue_.setValue(entry);
    qCInfo(LOG_BASIC) << "Abuse detection: User session expired, created/updated entry in settings";
}

bool MultipleAccountDetection_win::entryIsPresent(QString &username, QString &userId)
{
    SecretValue_win::TEntry entry;
    bool exists = secretValue_.isExistValue(entry);
    if (exists) {
        username = entry.username_;
        userId = entry.userId_;
    }
    return exists;
}

void MultipleAccountDetection_win::removeEntry()
{
    secretValue_.removeValue();
    qCInfo(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

