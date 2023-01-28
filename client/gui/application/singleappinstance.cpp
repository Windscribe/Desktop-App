#include "singleappinstance.h"
#include "singleappinstance_p.h"

#if defined(Q_OS_WINDOWS)
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
            debugOut("Connected to running Windscribe GUI instance.");
            client.abort();
            return true;
        }
    }

    debugOut("SingleAppInstance could not contact the running app instance.");

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
    #if defined(Q_OS_WINDOWS)

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
        socketName_ = QLatin1String("windscribe-singleappistance.socket");

        lockFile_.reset(new QLockFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                                      QLatin1String("/windscribe-singleappistance.lock")));
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
            debugOut("SingleAppInstance could not create the lock file. A new instance will be launched.");
        }
    }

    return false;

    #endif
}

void SingleAppInstancePrivate::release()
{
    #if defined(Q_OS_WINDOWS)
    appSingletonObj_.closeHandle();
    #else
    localServer_.close();
    if (!lockFile_.isNull()) {
        lockFile_->unlock();
    }
    #endif
}

void SingleAppInstancePrivate::debugOut(const char *format, ...)
{
    // The second instance of the app cannot use the Logger, as that would step on
    // the first instance's copy of the log.

    va_list arg_list;
    va_start(arg_list, format);

    QString sMsg;
    sMsg.vasprintf(format, arg_list);

    va_end(arg_list);

    if (sMsg.size() > 0)
    {
        QString sFilename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        sFilename += QLatin1String("/log_singleappinstanceguard.txt");

        QFile fileLog(sFilename);
        QFile::OpenMode openMode = QIODeviceBase::WriteOnly | QIODevice::Text;

        if (fileLog.size() > 524288) {
           openMode |= QIODevice::Truncate;
        }
        else {
           openMode |= QIODevice::Append;
        }

        if (fileLog.open(openMode))
        {
            QTextStream out(&fileLog);
            out << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << ' ' << sMsg << Qt::endl;
            fileLog.close();
        }
    }
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
