#include "ctrldmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "availableport.h"
#include "utils/executable_signature/executable_signature.h"


CtrldManager::CtrldManager(QObject *parent) : QObject(parent), bProcessStarted_(false)
{
    process_ = new QProcess(this);
    connect(process_, SIGNAL(started()), SLOT(onProcessStarted()));
    connect(process_, SIGNAL(finished(int)), SLOT(onProcessFinished()));
    connect(process_, SIGNAL(readyReadStandardOutput()), SLOT(onReadyReadStandardOutput()));
    connect(process_, SIGNAL(errorOccurred(QProcess::ProcessError)), SLOT(onProcessErrorOccurred(QProcess::ProcessError)));
    process_->setProcessChannelMode(QProcess::MergedChannels);

#if defined Q_OS_WIN
    ctrldExePath_ = QCoreApplication::applicationDirPath() + "/ctrld.exe";
    logPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "ctrld.log";
#elif defined Q_OS_MAC
    wstunelExePath_ = QCoreApplication::applicationDirPath() + "/../Helpers/windscribewstunnel";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(wstunelExePath_);
#elif defined Q_OS_LINUX
    wstunelExePath_ = QCoreApplication::applicationDirPath() + "/windscribewstunnel";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(wstunelExePath_);
#endif
    listenIp_ = "127.0.0.1";    // default listen ip for ctrld utility
}

CtrldManager::~CtrldManager()
{
    killProcess();
}

bool CtrldManager::runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains)
{
    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(ctrldExePath_.toStdWString())) {
        qCDebug(LOG_CTRLD) << "Failed to verify ctrld signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    QString ip = getAvailableIp();
    if (ip.isEmpty()) {
        qCDebug(LOG_CTRLD) << "ctrld cannot be started, all IP-addresses are occupied";
        return false;
    }

    inputArr_.clear();
    bProcessStarted_ = true;
    QStringList args;

    args << "run";
    args << "--listen=" + ip + ":53";
    args << "--primary_upstream=" + upstream1;
    if (!upstream2.isEmpty()) {
        args << "--secondary_upstream=" + upstream2;
        if (!domains.isEmpty()) {
            args << "--domains=" + domains.join(',');
        }
    }
    args << "--log" << logPath_;
    args << "-vv";
    process_->start(ctrldExePath_, args);
    return true;
}

void CtrldManager::killProcess()
{
    if (bProcessStarted_) {
        bProcessStarted_ = false;
        process_->close();

        // for Mac/Linux send kill command
    #if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
        QProcess killCmd(this);
        killCmd.execute("killall", QStringList() << "ctrld");
        killCmd.waitForFinished(-1);
    #endif

        process_->waitForFinished(-1);
        qCDebug(LOG_CTRLD) << "ctrld stopped";
    }
}

QString CtrldManager::listenIp() const
{
    return listenIp_;
}

void CtrldManager::onProcessStarted()
{
    qCDebug(LOG_CTRLD) << "ctrld started on " << listenIp_ + ":53";
}

void CtrldManager::onProcessFinished()
{
#ifdef Q_OS_WIN
    if (bProcessStarted_) {
        qCDebug(LOG_CTRLD) << "wstunnel finished";
        qCDebug(LOG_CTRLD) << process_->readAllStandardError();
        emit ctrldFinished();
    }
#endif
}

void CtrldManager::onReadyReadStandardOutput()
{
    inputArr_.append(process_->readAll());
    bool bSuccess = true;
    int length;
    while (true) {
        QString str = getNextStringFromInputBuffer(bSuccess, length);    
        if (bSuccess) {
            inputArr_.remove(0, length);
            qCDebug(LOG_CTRLD) << str;
        } else {
            break;
        }
    };
}

void CtrldManager::onProcessErrorOccurred(QProcess::ProcessError /*error*/)
{
    qCDebug(LOG_CTRLD) << "ctrld process error:" << process_->errorString();
}

QString CtrldManager::getNextStringFromInputBuffer(bool &bSuccess, int &outSize)
{
    QString str;
    bSuccess = false;
    outSize = 0;
    for (int i = 0; i < inputArr_.size(); ++i) {
        if (inputArr_[i] == '\n') {
            bSuccess = true;
            outSize = i + 1;
            return str.trimmed();
        }
        str += inputArr_[i];
    }

    return QString();
}

QString CtrldManager::getAvailableIp()
{
    if (!AvailablePort::isPortBusy(listenIp_, 53))
        return listenIp_;

    for (int i = 1; i <= 255; ++i) {
        QString ip = QString("127.0.0.%1").arg(i);
        if (ip != listenIp_ && !AvailablePort::isPortBusy(ip, 53)) {
            listenIp_ = ip;
            return listenIp_;
        }
    }
    // All IP-addresses are occupied
    listenIp_.clear();
    return "";
}

