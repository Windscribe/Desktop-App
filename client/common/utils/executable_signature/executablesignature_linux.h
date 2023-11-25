#pragma once

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);

private:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);

    friend class ExecutableSignature;
};
