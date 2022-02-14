#ifndef DETECTLANRANGE_H
#define DETECTLANRANGE_H

#include <QString>

class DetectLanRange
{
public:
    static bool isRfcLanRange(const QString &address);

private:
    static bool isRfcLoopbackAddress(quint32 ip);
    static bool isRfcPrivateAddress(quint32 ip);
};

#endif // DETECTLANRANGE_H
