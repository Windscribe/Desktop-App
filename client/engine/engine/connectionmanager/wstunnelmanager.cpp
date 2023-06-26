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
  : QObject(parent), helper_(helper), bProcessStarted_(false), bFirstMarketLineAfterStart_(false), port_(0)
{
#if defined Q_OS_WIN
    process_ = new QProcess(this);
    connect(process_, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(process_, SIGNAL(finished(int)), SLOT(onProcessFinished()));
    connect(process_, SIGNAL(readyReadStandardOutput()), SLOT(onReadyReadStandardOutput()));
    connect(process_, SIGNAL(errorOccurred(QProcess::ProcessError)), SLOT(onProcessErrorOccurred(QProcess::ProcessError)));
    process_->setProcessChannelMode(QProcess::MergedChannels);

    wstunnelExePath_ = QCoreApplication::applicationDirPath() + "/wstunnel.exe";
#endif
}

WstunnelManager::~WstunnelManager()
{
    killProcess();
}

bool WstunnelManager::runProcess(const QString &hostname, unsigned int port, bool isUdp)
{
    bool ret = false;
#if defined(Q_OS_WIN)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(wstunnelExePath_.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "Failed to verify wstunnel signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    inputArr_.clear();
    bFirstMarketLineAfterStart_ = true;
    bProcessStarted_ = true;
    QStringList args;
    QString addr = QString("127.0.0.1:%1:127.0.0.1:1194").arg(port_);
    QString hostaddr = QString("wss://%1:%2").arg(hostname).arg(port);
    args << "--localToRemote" << addr << hostaddr << "--verbose" << "--upgradePathPrefix=/";
    if (isUdp)
    {
        args << "--udp";
    }
    process_->start(wstunnelExePath_, args);
    ret = true;
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    ret = !helper_posix->startWstunnel(hostname, port, isUdp, port_);
    emit wstunnelStarted();
#endif
    qCDebug(LOG_BASIC) << "wstunnel started on port " << port_;
    return ret;
}

void WstunnelManager::killProcess()
{
#if defined(Q_OS_WIN)
    if (bProcessStarted_)
    {
        bProcessStarted_ = false;
        process_->close();
        process_->waitForFinished(-1);
    }
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetWStunnel);
#endif
    qCDebug(LOG_BASIC) << "wstunnel stopped";
}

unsigned int WstunnelManager::getPort()
{
    port_ = AvailablePort::getAvailablePort(DEFAULT_PORT);
    return port_;
}

void WstunnelManager::onProcessStarted()
{
    qCDebug(LOG_WSTUNNEL) << "wstunnel started";
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
}

void WstunnelManager::onReadyReadStandardOutput()
{
    inputArr_.append(process_->readAll());
    bool bSuccess = true;
    int length;
    while (true)
    {
        QString str = getNextStringFromInputBuffer(bSuccess, length);    
        if (bSuccess)
        {
            inputArr_.remove(0, length);
            qCDebug(LOG_WSTUNNEL) << str;

            if (bFirstMarketLineAfterStart_)
            {
                if (str.contains("WAIT for tcp connection on"))
                {
                    bFirstMarketLineAfterStart_ = false;
                    emit wstunnelStarted();
                }
            }
        }
        else
        {
            break;
        }
    };
}

void WstunnelManager::onProcessErrorOccurred(QProcess::ProcessError /*error*/)
{
    qCDebug(LOG_WSTUNNEL) << "wstunnel process error:" << process_->errorString();
}

QString WstunnelManager::getNextStringFromInputBuffer(bool &bSuccess, int &outSize)
{
    QString str;
    bSuccess = false;
    outSize = 0;
    for (int i = 0; i < inputArr_.size(); ++i)
    {
        if (inputArr_[i] == '\n')
        {
            bSuccess = true;
            outSize = i + 1;
            return str.trimmed();
        }
        str += inputArr_[i];
    }

    return QString();
}

