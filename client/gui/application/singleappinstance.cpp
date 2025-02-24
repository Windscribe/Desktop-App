#include "singleappinstance.h"
#include "singleappinstance_p.h"

#if defined(Q_OS_WIN)
#include <tchar.h>
#include "utils/winutils.h"
#else
#include <QLocalSocket>
#endif

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

namespace windscribe {


SingleAppInstancePrivate::SingleAppInstancePrivate() : QObject()
{
}

SingleAppInstancePrivate::~SingleAppInstancePrivate()
{
    release();
}

bool SingleAppInstancePrivate::activateRunningInstance()
{
    #if defined(Q_OS_WIN)

    HWND hwnd = WinUtils::appMainWindowHandle();
    if (hwnd) {
        ::SetForegroundWindow(hwnd);
        UINT dwActivateMessage = ::RegisterWindowMessage(WinUtils::wmActivateGui.c_str());
        ::PostMessage(hwnd, dwActivateMessage, 0, 0);
        return true;
    }

    return false;

    #else

    // Attempt to connect to the existing app instance, which will notify it to activate.
    QLocalSocket client;
    for (int i = 0; i < 3; ++i)
    {
        client.connectToServer(socketName_);
        if (client.waitForConnected(500))
        {
            client.abort();
            return true;
        }
    }

    // If we get here, the original instance is not responding or has crashed.
    lockFile_->removeStaleLockFile();
    lockFile_->tryLock();

    if (!localServer_.listen(socketName_))
    {
        if (localServer_.serverError() == QAbstractSocket::AddressInUseError)
        {
            QFile::remove(localServer_.fullServerName());
            return false;
        }
    }

    return true;

    #endif
}

bool SingleAppInstancePrivate::isRunning()
{
    #if defined(Q_OS_WIN)

    if (!appSingletonObj_.isValid())
    {
        appSingletonObj_.setHandle(::CreateEvent(NULL, TRUE, FALSE, _T("WINDSCRIBE_SINGLEAPPINSTANCE_EVENT")));

        if (!appSingletonObj_.isValid())
        {
            // Bad things are going on with the OS if we can't create the event.
            qDebug() << "SingleAppInstance could not create the app singleton event object.";
            return true;
        }

        if (::GetLastError() == ERROR_ALREADY_EXISTS)
        {
            appSingletonObj_.closeHandle();
            return true;
        }
    }

    return false;

    #else

    if (lockFile_.isNull())
    {
        socketName_ = "/var/run/windscribe/windscribe-singleappistance.socket";

        lockFile_.reset(new QLockFile("/var/run/windscribe/windscribe.lock"));
        lockFile_->setStaleLockTime(0);
        lockFile_->tryLock();

        localServer_.setSocketOptions(QLocalServer::UserAccessOption);
        connect(&localServer_, &QLocalServer::newConnection, this, &SingleAppInstancePrivate::anotherInstanceRunning);

        if (lockFile_->error() == QLockFile::LockFailedError) {
            return true;
        }

        if (lockFile_->error() == QLockFile::NoError)
        {
            // No existing lock was found, so we must be the only instance running.
            localServer_.listen(socketName_);
        }
    }

    return false;

    #endif
}

void SingleAppInstancePrivate::release()
{
    #if defined(Q_OS_WIN)
    appSingletonObj_.closeHandle();
    #else
    localServer_.close();
    if (!lockFile_.isNull()) {
        lockFile_->unlock();
    }
    #endif
}

SingleAppInstance::SingleAppInstance()
    : impl(new SingleAppInstancePrivate),
      activatedRunningInstance_(false)
{
    connect(impl.data(), &SingleAppInstancePrivate::anotherInstanceRunning, this, &SingleAppInstance::anotherInstanceRunning);
}

SingleAppInstance::~SingleAppInstance()
{
    release();
}

bool SingleAppInstance::isRunning()
{
    bool running = impl->isRunning();

    if (running) {
        activatedRunningInstance_ = impl->activateRunningInstance();
    }

    return running;
}

void SingleAppInstance::release()
{
    impl->release();
}

}
