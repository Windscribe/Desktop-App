#ifndef EXECUTABLE_SIGNATURE_WIN_H
#define EXECUTABLE_SIGNATURE_WIN_H

#include "executablesignature_p.h"

#include <Windows.h>
#include <wincrypt.h>

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);

private:
    bool verifyEmbeddedSignature(const std::wstring &exePath);
    bool checkWindscribeCertificate(PCCERT_CONTEXT pCertContext);
};

#endif // EXECUTABLE_SIGNATURE_WIN_H
