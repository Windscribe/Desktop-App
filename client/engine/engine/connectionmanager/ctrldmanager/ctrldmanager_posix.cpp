#include "ctrldmanager_posix.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

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

    if (helper_->startCtrld(addWsSuffix(upstream1), addWsSuffix(upstream2), domains, isCreateLog_)) {
        bProcessStarted_ = true;
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
