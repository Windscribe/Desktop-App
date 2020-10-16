#ifndef EXECUTABLE_SIGNATURE_H
#define EXECUTABLE_SIGNATURE_H

#include <QString>

// Check that the executable file/process is signed with the "Windscribe Limited" certificate
// Works only in release build
// The class has two implementations: ExecutableSignature_win and ExecutableSignature_mac
class ExecutableSignature
{
public:
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);

    static bool verifyWithSignCheck(const QString &executable);

};

#endif // EXECUTABLE_SIGNATURE_H
