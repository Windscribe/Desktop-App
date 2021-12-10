#ifndef EXECUTABLESIGNATURE_LINUX_H
#define EXECUTABLESIGNATURE_LINUX_H

#include "executablesignature_p.h"

class ExecutableSignaturePrivate : public ExecutableSignaturePrivateBase
{
public:
    explicit ExecutableSignaturePrivate(ExecutableSignature* const q);
    ~ExecutableSignaturePrivate();

    bool verify(const std::wstring &exePath);
    bool verify(const std::string &exePath);
};

#endif // EXECUTABLESIGNATURE_LINUX_H
