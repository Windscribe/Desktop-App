#ifndef AVAILABLEPORT_H
#define AVAILABLEPORT_H

#include <QString>

class AvailablePort
{
public:
    static unsigned int getAvailablePort(unsigned int defaultPort);
    static bool isPortBusy(const QString &ip, unsigned int port);
};

#endif // AVAILABLEPORT_H
