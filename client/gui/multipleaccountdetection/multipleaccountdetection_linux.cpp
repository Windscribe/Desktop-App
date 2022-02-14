#include "multipleaccountdetection_linux.h"
#include <QSettings>
#include <QDataStream>
#include "utils/logger.h"

MultipleAccountDetection_linux::MultipleAccountDetection_linux()
{
}

void MultipleAccountDetection_linux::userBecomeExpired(const QString &username)
{
    Q_UNUSED(username);
}

bool MultipleAccountDetection_linux::entryIsPresent(QString &username)
{
    Q_UNUSED(username);
    return false;
}

void MultipleAccountDetection_linux::removeEntry()
{
}
