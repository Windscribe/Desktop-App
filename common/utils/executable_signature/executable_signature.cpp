#include "executable_signature.h"

#ifdef Q_OS_WIN
    #include "executable_signature_win.h"
    #include "utils/hardcodedsettings.h"
#elif defined Q_OS_MAC
    #include "executable_signature_mac.h"
#elif defined Q_OS_LINUX
    #include "executablesignature_linux.h"
#endif

bool ExecutableSignature::isParentProcessGui()
{
#ifdef USE_SIGNATURE_CHECK
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::isParentProcessGui(HardcodedSettings::instance().windowsCertName());
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::isParentProcessGui();
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::isParentProcessGui();
    #endif
#else
    return true;
#endif
}

bool ExecutableSignature::verify(const QString &executablePath)
{
#ifdef USE_SIGNATURE_CHECK
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::verify(executablePath, HardcodedSettings::instance().windowsCertName());
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::verify(executablePath);
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::verify(executablePath);
    #endif
#else
    Q_UNUSED(executablePath);
    return true;
#endif
}

bool ExecutableSignature::verifyWithSignCheck(const QString &executable)
{
#ifdef USE_SIGNATURE_CHECK
    #ifdef Q_OS_WIN
        return ExecutableSignature_win::verify(executable, HardcodedSettings::instance().windowsCertName());
    #elif defined Q_OS_MAC
        return ExecutableSignature_mac::verifyWithSignCheck(executable);
    #elif defined Q_OS_LINUX
        return ExecutableSignature_linux::verify(executable);
    #endif
#else
    Q_UNUSED(executable);
    return true;
#endif
}
