#ifndef DETECTLOCALIP_H
#define DETECTLOCALIP_H

#include <QString>

class DetectLocalIP
{
public:
    static QString getLocalIP();

private:
#ifdef Q_OS_MAC
    static QString macGetPrimaryInterface();
    static QString macGetIpForInterface(const QString &interface);
    static QString macGetLocalIP();
#endif
};

#endif // DETECTLOCALIP_H
