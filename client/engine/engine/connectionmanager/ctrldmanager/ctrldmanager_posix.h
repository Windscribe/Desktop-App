#pragma once

#include "ictrldmanager.h"
#include "engine/helper/helper.h"

class CtrldManager_posix : public ICtrldManager
{
    Q_OBJECT
public:
    explicit CtrldManager_posix(QObject *parent, Helper *helper, bool isCreateLog);
    virtual ~CtrldManager_posix();

    bool runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains);
    void killProcess();
    QString listenIp() const;

private:
    Helper *helper_;
    bool bProcessStarted_;
    QString listenIp_;

    QString getAvailableIp();
};
