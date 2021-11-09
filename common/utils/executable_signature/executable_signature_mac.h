#ifndef EXECUTABLE_SIGNATURE_MAC_H
#define EXECUTABLE_SIGNATURE_MAC_H
#include <QString>

class ExecutableSignature_mac
{
public:
#ifdef QT_CORE_LIB
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
    static bool verifyWithSignCheck(const QString &executablePath);
#endif
};

#endif // EXECUTABLE_SIGNATURE_MAC_H
