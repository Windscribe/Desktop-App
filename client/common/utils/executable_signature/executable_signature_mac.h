#ifndef EXECUTABLE_SIGNATURE_MAC_H
#define EXECUTABLE_SIGNATURE_MAC_H

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

#endif // EXECUTABLE_SIGNATURE_MAC_H
