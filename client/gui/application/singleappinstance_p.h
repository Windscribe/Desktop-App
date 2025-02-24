#pragma once

#include <QObject>

#if defined(Q_OS_WIN)
#include "utils/win32handle.h"
#else
#include <QLockFile>
#include <QLocalServer>
#include <QScopedPointer>
#endif

namespace windscribe {

class SingleAppInstancePrivate : public QObject
{
    Q_OBJECT
public:
    explicit SingleAppInstancePrivate();
    ~SingleAppInstancePrivate();

    bool activateRunningInstance();
    bool isRunning();
    void release();

signals:
    void anotherInstanceRunning();

private:
    #if defined(Q_OS_WIN)
    wsl::Win32Handle appSingletonObj_;
    #else
    QString socketName_;
    QScopedPointer< QLockFile > lockFile_;
    QLocalServer localServer_;
    #endif
};

}
