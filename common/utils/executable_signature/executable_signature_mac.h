#ifndef EXECUTABLE_SIGNATURE_MAC_H
#define EXECUTABLE_SIGNATURE_MAC_H
#include <QString>

class ExecutableSignature_mac
{
public:
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
};

#endif // EXECUTABLE_SIGNATURE_MAC_H
