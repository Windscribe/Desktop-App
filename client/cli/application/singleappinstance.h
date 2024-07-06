#pragma once

#include <QLockFile>
#include <QLocalServer>
#include <QScopedPointer>

namespace windscribe {

class SingleAppInstance
{
public:
    explicit SingleAppInstance();
    ~SingleAppInstance();

    bool isRunning();
    void release();

private:
#if defined(Q_OS_LINUX)
    QScopedPointer<QLockFile> lockFile_;
    QLocalServer localServer_;
#endif
};

}
