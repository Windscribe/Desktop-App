#include "ctrldmanager_win.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/log/logger.h"
#include "utils/ws_assert.h"
#include "../availableport.h"
#include "utils/executable_signature/executable_signature.h"


CtrldManager_win::CtrldManager_win(QObject *parent, bool isCreateLog) : ICtrldManager(parent, isCreateLog), bProcessStarted_(false)
{
    process_ = new QProcess(this);
    connect(process_, &QProcess::started, this, &CtrldManager_win::onProcessStarted);
    connect(process_, &QProcess::finished, this, &CtrldManager_win::onProcessFinished);
    connect(process_, &QProcess::readyReadStandardOutput, this, &CtrldManager_win::onReadyReadStandardOutput);
    connect(process_, &QProcess::errorOccurred, this,&CtrldManager_win::onProcessErrorOccurred);
    process_->setProcessChannelMode(QProcess::MergedChannels);

    ctrldExePath_ = QCoreApplication::applicationDirPath() + "/windscribectrld.exe";
    logPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/ctrld.log";
    listenIp_ = "127.0.0.1";    // default listen ip for ctrld utility
}

CtrldManager_win::~CtrldManager_win()
{
    WS_ASSERT(!bProcessStarted_);
}

bool CtrldManager_win::runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains)
{
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ctrldExePath_.toStdWString())) {
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
    args << "--primary_upstream=" + addWsSuffix(upstream1);
    if (!upstream2.isEmpty()) {
        args << "--secondary_upstream=" + addWsSuffix(upstream2);
        if (!domains.isEmpty()) {
            args << "--domains=" + domains.join(',');
        }
    }
    if (isCreateLog_) {
        args << "--log" << logPath_;
        args << "-vv";
    }
    process_->start(ctrldExePath_, args);
    return true;
}

void CtrldManager_win::killProcess()
{
    if (bProcessStarted_) {
        bProcessStarted_ = false;
        process_->close();
        process_->waitForFinished(-1);
        qCDebug(LOG_CTRLD) << "ctrld stopped";
    }
}

QString CtrldManager_win::listenIp() const
{
    return listenIp_;
}

void CtrldManager_win::onProcessStarted()
{
    qCDebug(LOG_CTRLD) << "ctrld started on " << listenIp_ + ":53";
}

void CtrldManager_win::onProcessFinished()
{
    if (bProcessStarted_) {
        qCDebug(LOG_CTRLD) << "ctrld finished";
        qCDebug(LOG_CTRLD) << process_->readAllStandardError();
    }
}

void CtrldManager_win::onReadyReadStandardOutput()
{
    inputArr_.append(process_->readAll());
    bool bSuccess = true;
    int length;
    while (true) {
        QString str = getNextStringFromInputBuffer(bSuccess, length);
        if (bSuccess) {
            inputArr_.remove(0, length);
            if (isCreateLog_)
                qCDebug(LOG_CTRLD) << str;
        } else {
            break;
        }
    };
}

void CtrldManager_win::onProcessErrorOccurred(QProcess::ProcessError /*error*/)
{
    qCDebug(LOG_CTRLD) << "ctrld process error:" << process_->errorString();
}

QString CtrldManager_win::getNextStringFromInputBuffer(bool &bSuccess, int &outSize)
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

QString CtrldManager_win::getAvailableIp()
{
    // reset if we previously failed to find an available IP
    if (listenIp_.isEmpty()) {
        listenIp_ = "127.0.0.1";
    }

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

