#ifndef EXECUTABLE_SIGNATURE_WIN_H
#define EXECUTABLE_SIGNATURE_WIN_H

#include <Windows.h>
#include <wincrypt.h>
#include <string>

class ExecutableSignature;

class ExecutableSignaturePrivate
{
public:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);

private:
    bool verifyEmbeddedSignature(const std::wstring &exePath);
    bool checkWindscribeCertificate(PCCERT_CONTEXT pCertContext);

private:
    ExecutableSignature* const q_ptr;
};

#endif // EXECUTABLE_SIGNATURE_WIN_H
