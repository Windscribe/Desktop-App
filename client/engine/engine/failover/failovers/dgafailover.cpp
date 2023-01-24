#pragma once

#include "dgafailover.h"

#include <QCoreApplication>
#include <QLibrary>

#include "utils/logger.h"
#include "utils/executable_signature/executable_signature.h"


namespace failover {

void DgaFailover::getHostnames(bool)
{

#ifdef Q_OS_WIN
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/dga.dll";
#elif defined Q_OS_MAC
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/../Library/libdga.dylib";
#elif defined Q_OS_LINUX
    QString dgaLibPath = QCoreApplication::applicationDirPath() + "/libdga";
#endif

    if (!QFile::exists(dgaLibPath)) {
        qCDebug(LOG_BASIC) << "No dga, skip it";
        emit finished(FailoverRetCode::kFailed, QStringList());
        return;
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(dgaLibPath.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "Failed to verify dga signature: " << QString::fromStdString(sigCheck.lastError());
        emit finished(FailoverRetCode::kFailed, QStringList());
        return;
    }

    QLibrary lib(dgaLibPath);
    if (!lib.load()) {
        emit finished(FailoverRetCode::kFailed, QStringList());
        return;
    }

    typedef void (*FuncPrototype)(char *outStr, int length);
    FuncPrototype func = (FuncPrototype) lib.resolve("d1");
    char buf[1024] = {0};
    if (func) {
        func(buf, 1024);
    }
    lib.unload();

    QString domain = QString::fromStdString(buf);
    if (!domain.isEmpty())
    {
        qCDebug(LOG_FAILOVER) << "DGA domain:" << domain;      // TODO: remove
        emit finished(FailoverRetCode::kSuccess, QStringList() << domain);
    }
    else
        emit finished(FailoverRetCode::kFailed, QStringList());
}

} // namespace failover
