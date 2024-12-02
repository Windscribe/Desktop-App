#include "multipleaccountdetection_win.h"
#include "utils/log/categories.h"

MultipleAccountDetection_win::MultipleAccountDetection_win()
{
}

void MultipleAccountDetection_win::userBecomeExpired(const QString &username)
{
    qCInfo(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username;

    QString entry;
    if (!secretValue_.isExistValue(entry))
    {
        secretValue_.setValue(username);
        qCInfo(LOG_BASIC) << "Abuse detection: User session expired, created/updated entry in settings";
    }
}

bool MultipleAccountDetection_win::entryIsPresent(QString &username)
{
    return secretValue_.isExistValue(username);
}

void MultipleAccountDetection_win::removeEntry()
{
    secretValue_.removeValue();
    qCInfo(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

