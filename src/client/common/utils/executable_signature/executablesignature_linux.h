#pragma once

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);
    ~ExecutableSignaturePrivate() override;

    bool verify(const std::string& exePath) override;
    bool verify(const std::wstring& exePath) override;
};
