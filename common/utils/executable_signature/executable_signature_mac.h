#ifndef EXECUTABLE_SIGNATURE_MAC_H
#define EXECUTABLE_SIGNATURE_MAC_H

#include <string>

// Code that relies on Qt should be placed in QT_CORE_LIB guards
// There are helper projects that use this codebase that do not pull in Qt
#ifdef QT_CORE_LIB
#include <QString>
#endif

class ExecutableSignature_mac
{
public:
    bool verify(const std::string &exePath);

#ifdef QT_CORE_LIB
    static bool isParentProcessGui();
    static bool verify(const QString &executablePath);
    static bool verifyWithSignCheck(const QString &executablePath);
#endif

    const std::string& lastError() const { return lastError_; }

private:
    std::string lastError_;
};

#endif // EXECUTABLE_SIGNATURE_MAC_H
