#ifndef EXECUTABLESIGNATURE_LINUX_H
#define EXECUTABLESIGNATURE_LINUX_H

#ifdef QT_CORE_LIB
#include <QString>
#endif

class ExecutableSignature_linux
{
public:
#ifdef QT_CORE_LIB
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
#endif
};

#endif // EXECUTABLESIGNATURE_LINUX_H
