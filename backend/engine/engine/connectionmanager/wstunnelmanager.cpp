#include "wstunnelmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "Utils/logger.h"
#include "availableport.h"
#include "utils/executable_signature/executable_signature.h"


WstunnelManager::WstunnelManager(QObject *parent) : QObject(parent), bProcessStarted_(false)
{
    process_ = new QProcess(this);
    connect(process_, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(process_, SIGNAL(finished(int)), SLOT(onProcessFinished()));
    connect(process_, SIGNAL(readyReadStandardOutput()), SLOT(onReadyReadStandardOutput()));
    connect(process_, SIGNAL(errorOccurred(QProcess::ProcessError)), SLOT(onProcessErrorOccurred(QProcess::ProcessError)));
    process_->setProcessChannelMode(QProcess::MergedChannels);

#if defined Q_OS_WIN
    wstunelExePath_ = QCoreApplication::applicationDirPath() + "/wstunnel.exe";
#elif defined Q_OS_MAC
    wstunelExePath_ = QCoreApplication::applicationDirPath() + "/../Helpers/windscribewstunnel";
    qCDebug(LOG_BASIC) << wstunelExePath_;
#endif
}

WstunnelManager::~WstunnelManager()
{
    killProcess();
}

void WstunnelManager::runProcess(const QString &hostname, unsigned int port, bool isUdp)
{
    if (!ExecutableSignature::verify(wstunelExePath_))
    {
        return;
    }

    inputArr_.clear();
    bFirstMarketLineAfterStart_ = true;
    bProcessStarted_ = true;
    QStringList args;
    QString addr = QString("127.0.0.1:%1:127.0.0.1:1194").arg(port_);
    QString hostaddr = QString("wss://%1:%2").arg(hostname).arg(port);
    args << "-L" << addr << hostaddr << "-v";
    if (isUdp)
    {
        args << "-u";
    }
    process_->start(wstunelExePath_, args);
}

void WstunnelManager::killProcess()
{
    if (bProcessStarted_)
    {
        bProcessStarted_ = false;
        process_->close();

        // for Mac send kill command
    #if defined Q_OS_MAC
        QProcess killCmd(this);
        killCmd.execute("killall", QStringList() << "windscribewstunnel");
        killCmd.waitForFinished(-1);
    #endif

        process_->waitForFinished(-1);
        qCDebug(LOG_WSTUNNEL) << "wstunnel stopped";
    }
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

void WstunnelManager::onProcessErrorOccurred(QProcess::ProcessError error)
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

