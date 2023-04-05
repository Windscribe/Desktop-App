#include "ctrldmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "availableport.h"

CtrldManager::CtrldManager(QObject *parent, IHelper *helper) : QObject(parent), helper_(helper), bProcessStarted_(false)
{
#if defined Q_OS_WIN
    ctrldExePath_ = QCoreApplication::applicationDirPath() + "/windscribectrld.exe";
    logPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "ctrld.log";
#elif defined Q_OS_MAC
    ctrldExePath_ = QCoreApplication::applicationDirPath() + "/../Helpers/windscribectrld";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(ctrldExePath_);
    logPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/ctrld.log";
#elif defined Q_OS_LINUX
    ctrldExePath_ = QCoreApplication::applicationDirPath() + "/windscribectrld";
    qCDebug(LOG_BASIC) << Utils::cleanSensitiveInfo(ctrldExePath_);
    logPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/ctrld.log";
#endif
    listenIp_ = "127.0.0.1";    // default listen ip for ctrld utility
}

CtrldManager::~CtrldManager()
{
    killProcess();
}

bool CtrldManager::runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains)
{
    QFile::remove("\"" + logPath_ + "\"");

    QString ip = getAvailableIp();
    if (ip.isEmpty()) {
        qCDebug(LOG_CTRLD) << "ctrld cannot be started, all IP-addresses are occupied";
        return false;
    }

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
    // Probably don't log in the release
    //args << "--log" << "\"" + logPath_ + "\"";
    //args << "-vv";
#if defined Q_OS_WIN
    IHelper::ExecuteError err = helper_->startCtrld("windscribectrld.exe", args.join(' '));
#else
    IHelper::ExecuteError err = helper_->startCtrld("windscribectrld", args.join(' '));
#endif
    bProcessStarted_ = (err == IHelper::ExecuteError::EXECUTE_SUCCESS);
    if (bProcessStarted_) {
        qCDebug(LOG_CTRLD) << "ctrld started";
    }
    return bProcessStarted_;
}

void CtrldManager::killProcess()
{
    if (bProcessStarted_) {
        bProcessStarted_ = false;
        helper_->stopCtrld();
        qCDebug(LOG_CTRLD) << "ctrld stopped";
    }
}

QString CtrldManager::listenIp() const
{
    return listenIp_;
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

