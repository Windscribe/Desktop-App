#include "stunnelmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "availableport.h"
#include "utils/executable_signature/executable_signature.h"



StunnelManager::StunnelManager(QObject *parent) : QObject(parent), bProcessStarted_(false),
                                                  portForStunnel_(0)
{
    process_ = new QProcess(this);
    connect(process_, SIGNAL(finished(int)), SLOT(onStunnelProcessFinished()));

#if defined Q_OS_WIN
    stunelExePath_ = QCoreApplication::applicationDirPath() + "/tstunnel.exe";
#elif defined Q_OS_MAC
    stunelExePath_ = QCoreApplication::applicationDirPath() + "/../Helpers/windscribestunnel";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(stunelExePath_);
#elif defined Q_OS_LINUX
    stunelExePath_ = QCoreApplication::applicationDirPath() + "/windscribestunnel";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(stunelExePath_);
#endif

    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(strPath);
    dir.mkpath(strPath);
    path_ = strPath + "/stunnel.conf";
}

StunnelManager::~StunnelManager()
{
    killProcess();

    if (QFile::exists(path_)) {
        QFile::remove(path_);
    }
}

bool StunnelManager::runProcess()
{
    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(stunelExePath_.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "Failed to verify stunnel signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    bProcessStarted_ = true;
    QStringList args;
    args << path_;
    process_->start(stunelExePath_, args);
    qCDebug(LOG_BASIC) << "stunnel started";
    return true;
}

bool StunnelManager::setConfig(const QString &hostname, uint port)
{
    killProcess();
    if (makeConfigFile(hostname, port))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void StunnelManager::killProcess()
{
    if (bProcessStarted_)
    {
        bProcessStarted_ = false;
        process_->close();

        // for Mac/Linux send kill command
    #if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
        QProcess killCmd(this);
        killCmd.execute("killall", QStringList() << "windscribestunnel");
        killCmd.waitForFinished(-1);
    #endif

        process_->waitForFinished(-1);

        qCDebug(LOG_BASIC) << "stunnel stopped";
    }
}

unsigned int StunnelManager::getStunnelPort()
{
    return portForStunnel_;
}

void StunnelManager::onStunnelProcessFinished()
{
#ifdef Q_OS_WIN
    if (bProcessStarted_)
    {
        qCDebug(LOG_BASIC) << "Stunnel finished";
        qCDebug(LOG_BASIC) << process_->readAllStandardError();
        emit stunnelFinished();
    }
#endif
}

bool StunnelManager::makeConfigFile(const QString &hostname, uint port)
{
    QFile file(path_);
    if (file.open(QIODeviceBase::WriteOnly))
    {
        file.resize(0);

        portForStunnel_ = AvailablePort::getAvailablePort(DEFAULT_PORT);

        QString str;
        str = "[openvpn]\r\n";
        file.write(str.toLocal8Bit());
        str = "client = yes\r\n";
        file.write(str.toLocal8Bit());
        str = QString("accept = 127.0.0.1:%1\r\n").arg(portForStunnel_);
        file.write(str.toLocal8Bit());
        str = "connect = " + hostname + ":" + QString::number(port) + "\r\n";
        file.write(str.toLocal8Bit());

        file.close();

        qCDebug(LOG_BASIC) << "Used for stunnel:" << portForStunnel_;
        //qCDebug(LOG_BASIC) << "Stunnel config file: " << file.fileName();
        return true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Can't create stunnel config file";
    }
    return false;
}

