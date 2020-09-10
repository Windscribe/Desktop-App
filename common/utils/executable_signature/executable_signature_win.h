#ifndef EXECUTABLE_SIGNATURE_WIN_H
#define EXECUTABLE_SIGNATURE_WIN_H

#include <Windows.h>
#include <Softpub.h>
#include <wintrust.h>

#ifdef QT_CORE_LIB
#include <QString>
#endif

// check that the exe-file is signed with the "Windscribe Limited" certificate
class ExecutableSignature_win
{
public:
#ifdef QT_CORE_LIB
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
#endif
	static bool verify(const wchar_t *szExePath);

private:
    static bool verifyEmbeddedSignature(const wchar_t *pwszSourceFile);
    static bool checkWindscribeCertificate(PCCERT_CONTEXT pCertContext);
};

#endif // EXECUTABLE_SIGNATURE_WIN_H
