#ifndef EXECUTABLESIGNATURE_P_H
#define EXECUTABLESIGNATURE_P_H

#include <string>
#include <sstream>

class ExecutableSignature;

class ExecutableSignaturePrivateBase
{
public:
    explicit ExecutableSignaturePrivateBase(ExecutableSignature* const q) : q_ptr(q)
    {
    }

    virtual ~ExecutableSignaturePrivateBase()
    {
    }

    virtual bool verify(const std::wstring &exePath) = 0;
    virtual bool verify(const std::string &exePath) = 0;

    std::string lastError() const { return lastError_.str(); }

protected:
    ExecutableSignature* const q_ptr;
    std::ostringstream lastError_;
};

#endif // EXECUTABLESIGNATURE_P_H
