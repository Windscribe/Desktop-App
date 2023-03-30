#ifndef EXECUTABLESIGNATURE_LINUX_H
#define EXECUTABLESIGNATURE_LINUX_H

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

#endif // EXECUTABLESIGNATURE_LINUX_H
