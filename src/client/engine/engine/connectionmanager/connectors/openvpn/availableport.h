#pragma once

#include <QString>

class AvailablePort
{
public:
    static unsigned int getAvailablePort(unsigned int defaultPort);
    static bool isPortBusy(const QString &ip, unsigned int port);
};
