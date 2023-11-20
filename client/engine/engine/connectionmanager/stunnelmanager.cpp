#include "stunnelmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "availableport.h"
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
#include "engine/helper/helper_posix.h"
#endif
#include "utils/executable_signature/executable_signature.h"
#include "utils/logger.h"
#include "utils/extraconfig.h"

StunnelManager::StunnelManager(QObject *parent, IHelper *helper)
  : QObject(parent), helper_(helper), bProcessStarted_(false), portForStunnel_(0)
{
#if defined Q_OS_WIN
    process_ = new QProcess(this);
    connect(process_, SIGNAL(finished(int)), SLOT(onStunnelProcessFinished()));

    stunnelExePath_ = QCoreApplication::applicationDirPath() + "/tstunnel.exe";

    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(strPath);
    dir.mkpath(strPath);
    path_ = strPath + "/stunnel.conf";
#endif
}

StunnelManager::~StunnelManager()
{
    killProcess();

#if defined Q_OS_WIN
    if (QFile::exists(path_)) {
        QFile::remove(path_);
    }
#endif
}

bool StunnelManager::runProcess()
{
    bool ret = false;

#if defined(Q_OS_WIN)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(stunnelExePath_.toStdWString())) {
        qCDebug(LOG_BASIC) << "Failed to verify stunnel signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    bProcessStarted_ = true;
    QStringList args;
    args << path_;
    process_->start(stunnelExePath_, args);
    ret = true;
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    ret = !helper_posix->startStunnel();
#endif
    qCDebug(LOG_BASIC) << "stunnel started on port " << portForStunnel_;
    return ret;
}

bool StunnelManager::setConfig(const QString &hostname, uint port)
{
    killProcess();

#if defined(Q_OS_WIN)
    if (makeConfigFile(hostname, port)) {
        return true;
    } else {
        return false;
    }
#else
    portForStunnel_ = AvailablePort::getAvailablePort(DEFAULT_PORT);
    bool extraPadding = ExtraConfig::instance().getStealthExtraTLSPadding();

    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    return !helper_posix->configureStunnel(hostname, port, portForStunnel_ ,extraPadding);
#endif
}

void StunnelManager::killProcess()
{
#if defined(Q_OS_WIN)
    if (bProcessStarted_) {
        bProcessStarted_ = false;
        process_->close();
        process_->waitForFinished(-1);
    }
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetStunnel);
#endif
    qCDebug(LOG_BASIC) << "stunnel stopped";
}

unsigned int StunnelManager::getStunnelPort()
{
    return portForStunnel_;
}

void StunnelManager::onStunnelProcessFinished()
{
#ifdef Q_OS_WIN
    if (bProcessStarted_) {
        qCDebug(LOG_BASIC) << "Stunnel finished";
        qCDebug(LOG_BASIC) << process_->readAllStandardError();
        emit stunnelFinished();
    }
#endif
}

#ifdef Q_OS_WIN
bool StunnelManager::makeConfigFile(const QString &hostname, uint port)
{
    QFile file(path_);
    if (file.open(QIODevice::WriteOnly)) {
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
        if (ExtraConfig::instance().getStealthExtraTLSPadding()) {
            str = "options = TLSEXT_PADDING\r\noptions = TLSEXT_PADDING_SUPER\r\n";
            file.write(str.toLocal8Bit());
        }

        file.close();

        qCDebug(LOG_BASIC) << "Used for stunnel:" << portForStunnel_;
        //qCDebug(LOG_BASIC) << "Stunnel config file: " << file.fileName();
        return true;
    } else {
        qCDebug(LOG_BASIC) << "Can't create stunnel config file";
    }
    return false;
}
#endif

