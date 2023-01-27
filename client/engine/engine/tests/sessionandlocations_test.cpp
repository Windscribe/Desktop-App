#include "sessionandlocations_test.h"

#include <QFile>

QByteArray SessionAndLocationsTest::getSessionData()
{
    QFile file;
    if (isFreeSessionNow_)
    {
        file.setFileName("c:\\5\\session_free.api");
    }
    else
    {
        file.setFileName("c:\\5\\session_pro.api");
    }
    if (file.open(QIODeviceBase::ReadOnly))
    {
        return file.readAll();
    }
    else
    {
        Q_ASSERT(false);
        return QByteArray();
    }
}

QByteArray SessionAndLocationsTest::getLocationsData()
{
    QFile file;
    if (isFreeSessionNow_)
    {
        file.setFileName("c:\\5\\locations_free.api");
    }
    else
    {
        file.setFileName("c:\\5\\locations_pro.api");
    }
    if (file.open(QIODeviceBase::ReadOnly))
    {
        return file.readAll();
    }
    else
    {
        Q_ASSERT(false);
        return QByteArray();
    }
}

bool SessionAndLocationsTest::toggleSessionStatus()
{
    isFreeSessionNow_ = !isFreeSessionNow_;
    return isFreeSessionNow_;
}

SessionAndLocationsTest::SessionAndLocationsTest() : isFreeSessionNow_(false)
{
}
