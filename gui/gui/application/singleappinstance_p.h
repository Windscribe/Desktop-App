#ifndef SINGLE_APP_INSTANCE_P_H
#define SINGLE_APP_INSTANCE_P_H

#include <QObject>

#if defined(Q_OS_WINDOWS)
#include <qt_windows.h>
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

Q_SIGNALS:
    void anotherInstanceRunning();

private:
    #if defined(Q_OS_WINDOWS)
    HANDLE hEventCurrentApp_;
    #else
    QString socketName_;
    QScopedPointer< QLockFile > lockFile_;
    QLocalServer localServer_;
    #endif

    void debugOut(const char *format, ...);
};

}

#endif // SINGLE_APP_INSTANCE_P_H
