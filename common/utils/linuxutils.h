#ifndef LINUXUTILS_H
#define LINUXUTILS_H

#include <QString>
#include <QMap>


namespace LinuxUtils
{
    QString getOsVersionString();
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName);
}


#endif // LINUXUTILS_H
