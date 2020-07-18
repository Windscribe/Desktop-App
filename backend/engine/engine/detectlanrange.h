#ifndef DETECTLANRANGE_H
#define DETECTLANRANGE_H

#include <QByteArray>
#include <QStringList>

class DetectLanRange
{
public:
    static DetectLanRange &instance()
    {
        static DetectLanRange s;
        return s;
    }

    bool isRfcLanRange();
    QStringList getIps();

private:
    DetectLanRange();

    bool isRfcRangeAddress(quint32 ip);
    QStringList ips_;
};

#endif // DETECTLANRANGE_H
