#ifndef EXECUTABLE_SIGNATURE_MAC_H
#define EXECUTABLE_SIGNATURE_MAC_H

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);

    bool verifyWithSignCheck(const std::wstring &exePath);
};

#endif // EXECUTABLE_SIGNATURE_MAC_H
