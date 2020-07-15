#ifndef EXECUTABLE_SIGNATURE_WIN_H
#define EXECUTABLE_SIGNATURE_WIN_H

#include <Windows.h>
#include <Softpub.h>
#include <wintrust.h>
#include <QString>

// check that the exe-file is signed with the "Windscribe Limited" certificate
class ExecutableSignature_win
{
public:
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);

private:
    static bool verify(const wchar_t *szExePath);
    static bool verifyEmbeddedSignature(const wchar_t *pwszSourceFile);
    static bool checkWindscribeCertificate(PCCERT_CONTEXT pCertContext);
};

#endif // EXECUTABLE_SIGNATURE_WIN_H
