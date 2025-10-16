#include "executablesignature_linux.h"

ExecutableSignaturePrivate::ExecutableSignaturePrivate(ExecutableSignature* const q) : ExecutableSignaturePrivateBase(q)
{
}

ExecutableSignaturePrivate::~ExecutableSignaturePrivate()
{
}

bool ExecutableSignaturePrivate::verify(const std::string& exePath)
{
    // Stub implementation - signature verification removed for Linux
    (void)exePath;
    return true;
}

bool ExecutableSignaturePrivate::verify(const std::wstring& exePath)
{
    // Stub implementation - signature verification removed for Linux
    (void)exePath;
    return true;
}
