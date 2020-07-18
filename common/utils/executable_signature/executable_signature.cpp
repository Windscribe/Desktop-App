#include "executable_signature.h"

#ifdef Q_OS_WIN
    #include "executable_signature_win.h"
#else
    #include "executable_signature_win.h"
#endif

bool ExecutableSignature::isParentProcessGui()
{
#ifdef QT_DEBUG
    return true;
#else
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::isParentProcessGui();
    #else
        return ExecutableSignature_mac::isParentProcessGui();
    #endif
#endif
}

bool ExecutableSignature::verify(const QString &executablePath)
{
#ifdef QT_DEBUG
    return true;
#else
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::verify(executablePath);
    #else
        return ExecutableSignature_mac::verify(executablePath);
    #endif
#endif
}
