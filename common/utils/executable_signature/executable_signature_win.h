#ifndef EXECUTABLE_SIGNATURE_WIN_H
#define EXECUTABLE_SIGNATURE_WIN_H

#include "executablesignature_p.h"

#include <Windows.h>
#include <wincrypt.h>

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);

private:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);

    bool verifyEmbeddedSignature(const std::wstring &exePath);
    bool checkWindscribeCertificate(PCCERT_CONTEXT pCertContext);

    friend class ExecutableSignature;
};

#endif // EXECUTABLE_SIGNATURE_WIN_H
