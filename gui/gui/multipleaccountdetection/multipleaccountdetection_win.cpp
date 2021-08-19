#include "multipleaccountdetection_win.h"
#include "Utils/logger.h"

MultipleAccountDetection_win::MultipleAccountDetection_win()
{
}

void MultipleAccountDetection_win::userBecomeExpired(const QString &username)
{
    qCDebug(LOG_BASIC) << "MultipleAccountDetection::userBecomeExpired, username =" << username;

    QString entry;
    if (!secretValue_.isExistValue(entry))
    {
        secretValue_.setValue(username);
        qCDebug(LOG_BASIC) << "Abuse detection: User session expired, created/updated entry in settings";
    }
}

bool MultipleAccountDetection_win::entryIsPresent(QString &username)
{
   return secretValue_.isExistValue(username);
}

void MultipleAccountDetection_win::removeEntry()
{
    secretValue_.removeValue();
    qCDebug(LOG_BASIC) << "Abuse detection: Entry for abuse detection removed";
}

