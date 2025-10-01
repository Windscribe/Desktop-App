#include "singleappinstance.h"

#if defined(Q_OS_LINUX)
#include <QLocalSocket>
#endif

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include "utils/log/categories.h"

namespace windscribe {

bool SingleAppInstance::isRunning()
{
#if defined(Q_OS_LINUX)
    if (lockFile_.isNull()) {
        lockFile_.reset(new QLockFile("/var/run/windscribe/windscribe.lock"));
        lockFile_->setStaleLockTime(0);
        lockFile_->tryLock();

        localServer_.setSocketOptions(QLocalServer::UserAccessOption);

        if (lockFile_->error() == QLockFile::LockFailedError) {
            return true;
        }

        if (lockFile_->error() != QLockFile::NoError) {
            qCInfo(LOG_BASIC) << "SingleAppInstance could not create the lock file. A new instance will be launched.";
        }
    }
#endif
    return false;
}

void SingleAppInstance::release()
{
#if defined(Q_OS_LINUX)
    localServer_.close();
    if (!lockFile_.isNull()) {
        lockFile_->unlock();
    }
#endif
}

SingleAppInstance::SingleAppInstance()
{
}

SingleAppInstance::~SingleAppInstance()
{
    release();
}

}

