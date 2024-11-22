#include "stunnelmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "availableport.h"
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
#include "engine/helper/helper_posix.h"
#endif
#include "utils/executable_signature/executable_signature.h"
#include "utils/log/categories.h"
#include "utils/extraconfig.h"

StunnelManager::StunnelManager(QObject *parent, IHelper *helper)
  : QObject(parent), helper_(helper), port_(0), bProcessStarted_(false)
{
#if defined Q_OS_WIN
    process_ = new QProcess(this);
    connect(process_, &QProcess::started, this, &StunnelManager::onProcessStarted);
    connect(process_, &QProcess::finished, this, &StunnelManager::onProcessFinished);
    connect(process_, &QProcess::readyReadStandardOutput, this, &StunnelManager::onProcessReadyRead);
    connect(process_, &QProcess::errorOccurred, this, &StunnelManager::onProcessErrorOccurred);
    process_->setProcessChannelMode(QProcess::MergedChannels);

    stunnelExePath_ = QCoreApplication::applicationDirPath() + "/windscribewstunnel.exe";
#endif
}

StunnelManager::~StunnelManager()
{
    killProcess();
}

bool StunnelManager::runProcess(const QString &hostname, unsigned int port, bool isExtraPadding)
{
    bool ret = false;

#if defined(Q_OS_WIN)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(stunnelExePath_.toStdWString())) {
        qCDebug(LOG_BASIC) << "Failed to verify stunnel signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    QStringList args;
    args << path_;
    QString addr = QString("127.0.0.1:%1").arg(port_);
    QString hostaddr = QString("https://%1:%2").arg(hostname).arg(port);
    args << "--listenAddress" << addr;
    args << "--remoteAddress" << hostaddr;
    args << "--logFilePath" << "";
    args << "--tunnelType" << "2";
    if (isExtraPadding) {
        args << "--extraTlsPadding";
    }

    process_->start(stunnelExePath_, args);
    ret = true;
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);

    ret = !helper_posix->startStunnel(hostname, port, port_, isExtraPadding);
    if (ret) {
        emit stunnelStarted();
    }
#endif
    if (ret) {
        qCDebug(LOG_BASIC) << "stunnel started on port " << port_;
    } else {
        qCDebug(LOG_BASIC) << "stunnel failed to start";
    }
    bProcessStarted_ = ret;
    return ret;
}

void StunnelManager::killProcess()
{
#if defined(Q_OS_WIN)
    if (bProcessStarted_) {
        process_->close();
        process_->waitForFinished(-1);
    }
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetStunnel);
#endif
    bProcessStarted_ = false;
    qCDebug(LOG_BASIC) << "stunnel stopped";
}

unsigned int StunnelManager::getPort()
{
    port_ = AvailablePort::getAvailablePort(kDefaultPort);
    return port_;
}

void StunnelManager::onProcessStarted()
{
    qCDebug(LOG_BASIC) << "stunnel started";
    emit stunnelStarted();
}

void StunnelManager::onProcessFinished()
{
#ifdef Q_OS_WIN
    if (bProcessStarted_) {
        qCDebug(LOG_BASIC) << "Stunnel finished";
        qCDebug(LOG_BASIC) << process_->readAllStandardError();
        emit stunnelFinished();
    }
#endif
    bProcessStarted_ = false;
}

void StunnelManager::onProcessErrorOccurred(QProcess::ProcessError /*error*/)
{
    qCDebug(LOG_BASIC) << "stunnel process error:" << process_->errorString();
}

void StunnelManager::onProcessReadyRead()
{
    QStringList strs = QString(process_->readAll()).split("\n", Qt::SkipEmptyParts);
    for (auto str : strs) {
        qCDebug(LOG_WSTUNNEL) << str;
    }
}
