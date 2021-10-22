#include "singleappinstance.h"
#include "singleappinstance_p.h"

#if defined(Q_OS_WINDOWS)
#include <tchar.h>
#include "utils/winutils.h"
#else
#include <QLocalSocket>
#include <QStandardPaths>
#endif

namespace windscribe {


SingleAppInstancePrivate::SingleAppInstancePrivate() : QObject()
{
    #if defined(Q_OS_WINDOWS)
    hEventCurrentApp_ = NULL;
    #endif
}

SingleAppInstancePrivate::~SingleAppInstancePrivate()
{
    release();
}

bool SingleAppInstancePrivate::activateRunningInstance()
{
    #if defined(Q_OS_WINDOWS)

    HWND hwnd = ::FindWindow(WinUtils::classNameIcon.c_str(), WinUtils::wsGuiIcon.c_str());
    if (hwnd)
    {
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
    localServer_.listen(socketName_);

    qDebug() << "SingleAppInstance could not contact the running app instance.";

    return true;

    #endif
}

bool SingleAppInstancePrivate::isRunning()
{
    #if defined(Q_OS_WINDOWS)

    if (hEventCurrentApp_ == NULL)
    {
        hEventCurrentApp_ = ::CreateEvent(NULL, TRUE, FALSE, _T("WINDSCRIBE_SINGLEAPPINSTANCE_EVENT"));

        if (hEventCurrentApp_ == NULL) {
            // Bad things are going on with the OS if we can't create the event.
            qDebug() << "SingleAppInstance could not create the app singleton event object.";
            return true;
        }

        if (::GetLastError() == ERROR_ALREADY_EXISTS)
        {
            ::CloseHandle(hEventCurrentApp_);
            hEventCurrentApp_ = NULL;
            return true;
        }
    }

    return false;

    #else

    QString lockName("windscribe-singleappistance.lock");
    socketName_ = QLatin1String("windscribe-singleappistance.socket");

    lockFile_.reset(new QLockFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + lockName));
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
    else {
        qDebug() << "SingleAppInstance could not create the lock file. A new instance will be launched.";
    }

    return false;

    #endif
}

void SingleAppInstancePrivate::release()
{
    #if defined(Q_OS_WINDOWS)
    if (hEventCurrentApp_ != NULL)
    {
        ::CloseHandle(hEventCurrentApp_);
        hEventCurrentApp_ = NULL;
    }
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
