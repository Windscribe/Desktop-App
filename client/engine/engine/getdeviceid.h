#ifndef GETDEVICEID_H
#define GETDEVICEID_H

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

#endif // GETDEVICEID_H
