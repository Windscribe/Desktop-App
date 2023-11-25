#pragma once

#include <Windows.h>
#include <wincrypt.h>

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);

private:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);

    bool verifyEmbeddedSignature(const std::wstring &exePath);
    bool checkCertificate(PCCERT_CONTEXT certContext);

    friend class ExecutableSignature;
};
