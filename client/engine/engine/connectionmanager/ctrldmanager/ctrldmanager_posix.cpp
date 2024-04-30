#include "ctrldmanager_posix.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/ws_assert.h"
#include "../availableport.h"

CtrldManager_posix::CtrldManager_posix(QObject *parent, IHelper *helper, bool isCreateLog) : ICtrldManager(parent, isCreateLog), helper_(helper), bProcessStarted_(false)
{
    listenIp_ = "127.0.0.1";    // default listen ip for ctrld utility
}

CtrldManager_posix::~CtrldManager_posix()
{
    WS_ASSERT(!bProcessStarted_);
}

bool CtrldManager_posix::runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains)
{
    WS_ASSERT(!bProcessStarted_);

    QString ip = getAvailableIp();
    if (ip.isEmpty()) {
        qCDebug(LOG_CTRLD) << "ctrld cannot be started, all IP-addresses are occupied";
        return false;
    }

    IHelper::ExecuteError err = helper_->startCtrld(ip, addWsSuffix(upstream1), addWsSuffix(upstream2), domains, isCreateLog_);
    bProcessStarted_ = (err == IHelper::ExecuteError::EXECUTE_SUCCESS);
    if (bProcessStarted_) {
        qCDebug(LOG_CTRLD) << "ctrld started";
    }
    return bProcessStarted_;
}

void CtrldManager_posix::killProcess()
{
    if (bProcessStarted_) {
        bProcessStarted_ = false;
        helper_->stopCtrld();
        qCDebug(LOG_CTRLD) << "ctrld stopped";
    }
}

QString CtrldManager_posix::listenIp() const
{
    return listenIp_;
}


QString CtrldManager_posix::getAvailableIp()
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

