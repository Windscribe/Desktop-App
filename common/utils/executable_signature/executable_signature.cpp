#include "executable_signature.h"

#ifdef Q_OS_WIN
    #include "executable_signature_win.h"
#elif defined Q_OS_MAC
    #include "executable_signature_mac.h"
#elif defined Q_OS_LINUX
    #include "executablesignature_linux.h"
#endif

bool ExecutableSignature::isParentProcessGui()
{
#ifdef QT_DEBUG
    return true;
#else
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::isParentProcessGui();
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::isParentProcessGui();
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::isParentProcessGui();
    #endif
#endif
}

bool ExecutableSignature::verify(const QString &executablePath)
{
#ifdef QT_DEBUG
    Q_UNUSED(executablePath);
    return true;
#else
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::verify(executablePath);
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::verify(executablePath);
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::verify(executablePath);
    #endif
#endif
}

bool ExecutableSignature::verifyWithSignCheck(const QString &executable)
{
#ifdef QT_DEBUG
    Q_UNUSED(executable);
    return true;
#else
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::verify(executable);
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::verifyWithSignCheck(executable);
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::verify(executable);
    #endif
#endif
}
