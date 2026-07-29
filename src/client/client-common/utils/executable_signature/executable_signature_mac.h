#pragma once

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);
    bool verifyWithBundleId(const std::string &bundlePath, const std::string &bundleId);

private:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);

    // Shared body of the checks above. requirementString is the complete requirement to validate
    // against, so the identifier clause can be added without duplicating the validation flags.
    bool verifyAgainst(const std::string &exePath, const std::string &requirementString);

    friend class ExecutableSignature;
};
