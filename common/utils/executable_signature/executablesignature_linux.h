#ifndef EXECUTABLESIGNATURE_LINUX_H
#define EXECUTABLESIGNATURE_LINUX_H

#include <string>

// Code that relies on Qt should be placed in QT_CORE_LIB guards
// There are helper projects that use this codebase that do not pull in Qt
#ifdef QT_CORE_LIB
#include <QString>
#endif

class ExecutableSignature_linux
{
public:
    bool verifyWithPublicKey(const std::string &exePath, const std::string &sigPath, const std::string &pubKeyBytes);

#ifdef QT_CORE_LIB
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
    static bool verifyWithPublicKeyFromResources(const QString &executablePath, const QString &signaturePath, const QString &publicKeyPath);
    static bool verifyWithPublicKeyFromFilesystem(const QString &executablePath, const QString &signaturePath, const QString &publicKeyPath);
#endif

    const std::string& lastError() const { return lastError_; }

private:
    std::string lastError_;
};

#endif // EXECUTABLESIGNATURE_LINUX_H
