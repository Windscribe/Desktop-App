#include "linuxutils.h"
#include <sys/utsname.h>

QString LinuxUtils::getOsVersionString()
{
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        return QString(unameData.sysname) + " " + QString(unameData.version);
    }
    else
    {
        return "Can't detect OS Linux version";
    }
}
