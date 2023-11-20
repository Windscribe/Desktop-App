#include "wstunnelmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "availableport.h"
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
#include "engine/helper/helper_posix.h"
#endif
#include "utils/executable_signature/executable_signature.h"


WstunnelManager::WstunnelManager(QObject *parent, IHelper *helper)
  : QObject(parent), helper_(helper), bProcessStarted_(false), port_(0)
{
#if defined Q_OS_WIN
    process_ = new QProcess(this);
    connect(process_, &QProcess::started, this, &WstunnelManager::onProcessStarted);
    connect(process_, &QProcess::finished, this, &WstunnelManager::onProcessFinished);
    connect(process_, &QProcess::readyReadStandardOutput, this, &WstunnelManager::onProcessReadyRead);
    connect(process_, &QProcess::errorOccurred, this, &WstunnelManager::onProcessErrorOccurred);

    process_->setProcessChannelMode(QProcess::MergedChannels);
    wstunnelExePath_ = QCoreApplication::applicationDirPath() + "/windscribewstunnel.exe";
#endif
}

WstunnelManager::~WstunnelManager()
{
    killProcess();
}

bool WstunnelManager::runProcess(const QString &hostname, unsigned int port)
{
    bool ret = false;
#if defined(Q_OS_WIN)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(wstunnelExePath_.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "Failed to verify wstunnel signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    QStringList args;
    QString addr = QString("127.0.0.1:%1").arg(port_);
    QString hostaddr = QString("wss://%1:%2/tcp/127.0.0.1/1194").arg(hostname).arg(port);
    args << "--listenAddress" << addr;
    args << "--remoteAddress" << hostaddr;
    args << "--logFilePath" << "";
    process_->start(wstunnelExePath_, args);
    ret = true;
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    ret = !helper_posix->startWstunnel(hostname, port, port_);
    if (ret) {
        emit wstunnelStarted();
    }
#endif
    if (ret) {
        qCDebug(LOG_BASIC) << "wstunnel started on port " << port_;
    } else {
        qCDebug(LOG_BASIC) << "wstunnel failed to start";
    }

    bProcessStarted_ = ret;
    return ret;
}

void WstunnelManager::killProcess()
{
#if defined(Q_OS_WIN)
    if (bProcessStarted_)
    {
        process_->close();
        process_->waitForFinished(-1);
    }
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetWStunnel);
#endif
    bProcessStarted_ = false;
    qCDebug(LOG_BASIC) << "wstunnel stopped";
}

unsigned int WstunnelManager::getPort()
{
    port_ = AvailablePort::getAvailablePort(kDefaultPort);
    return port_;
}

void WstunnelManager::onProcessStarted()
{
    qCDebug(LOG_WSTUNNEL) << "wstunnel started";
    emit wstunnelStarted();
}

void WstunnelManager::onProcessFinished()
{
#ifdef Q_OS_WIN
    if (bProcessStarted_)
    {
        qCDebug(LOG_WSTUNNEL) << "wstunnel finished";
        qCDebug(LOG_WSTUNNEL) << process_->readAllStandardError();
        emit wstunnelFinished();
    }
#endif
    bProcessStarted_ = false;
}

void WstunnelManager::onProcessErrorOccurred(QProcess::ProcessError /*error*/)
{
    qCDebug(LOG_WSTUNNEL) << "wstunnel process error:" << process_->errorString();
}

void WstunnelManager::onProcessReadyRead()
{
    QStringList strs = QString(process_->readAll()).split("\n", Qt::SkipEmptyParts);
    for (auto str : strs) {
        qCDebug(LOG_WSTUNNEL) << str;
    }
}
