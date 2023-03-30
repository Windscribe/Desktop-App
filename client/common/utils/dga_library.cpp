#include "dga_library.h"

#include <QCoreApplication>
#include <QFile>

#include "utils/logger.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/ws_assert.h"


DgaLibrary::~DgaLibrary()
{
    if (lib) {
        lib->unload();
        delete lib;
    }
}

bool DgaLibrary::load()
{
    static bool alreadyLogVersion = false;
#ifdef Q_OS_WIN
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/dga.dll";
#elif defined Q_OS_MAC
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/../Helpers/libdga.dylib";
#elif defined Q_OS_LINUX
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/lib/libdga.so";
#endif

    if (!QFile::exists(dgaLibPath)) {
        qCDebug(LOG_BASIC) << "No dga found";
        return false;
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(dgaLibPath.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "Failed to verify dga signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    lib = new QLibrary(dgaLibPath);
    if (!lib->load()) {
        qCDebug(LOG_BASIC) << "Failed to load dga";
        return false;
    }

    func = (FuncPrototype) lib->resolve("d1");
    if (!func) {
        qCDebug(LOG_BASIC) << "Failed to resolve dga function";
        return false;
    }

    if (!alreadyLogVersion) {
        char buf[1024] = {0};
        func(buf, 1024, PAR_LIBRARY_VERSION, nullptr, nullptr, nullptr);
        qCDebug(LOG_BASIC) << "dga version:" << buf;
        alreadyLogVersion = true;
    }
    return true;
}

QString DgaLibrary::getParameter(int par)
{
    char buf[1024] = {0};
    if (func)
        func(buf, 1024, par, nullptr, nullptr, nullptr);
    QString ret = buf;
    WS_ASSERT(!ret.isEmpty());
    return ret;
}
