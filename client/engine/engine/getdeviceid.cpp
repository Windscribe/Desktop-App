#include "getdeviceid.h"
#include <QSettings>
#include <QUuid>

QString GetDeviceId::getDeviceId()
{
    QMutexLocker locker(&mutex_);

    QSettings s;
    if (s.contains("deviceId"))
    {
        return s.value("deviceId").toString();
    }
    else
    {
        QUuid id = QUuid::createUuid();
        QString strId = id.toString();
        strId = strId.remove(0, 1);
        strId = strId.remove(strId.length() - 1, 1);
        s.setValue("deviceId", strId);
        return strId;
    }
}

GetDeviceId::GetDeviceId()
{

}
