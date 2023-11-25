#pragma once

#include <QString>
#include <QMutex>

class GetDeviceId
{
public:
    static GetDeviceId &instance()
    {
        static GetDeviceId s;
        return s;
    }

    QString getDeviceId();

private:
    GetDeviceId();
    QMutex mutex_;
};
