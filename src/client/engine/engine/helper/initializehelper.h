#pragma once

#include <QObject>

#include "types/enums.h"
#include "engine/helper/ihelperbackend.h"

// wait for initialize helper
class InitializeHelper : public QObject
{
    Q_OBJECT
public:
    explicit InitializeHelper(QObject *parent, IHelperBackend *helperBackend);
    void start();

signals:
    void finished(INIT_HELPER_RET ret);

private slots:
    void onTimerControlHelper();
    void handleHelperInit();

private:
    IHelperBackend *helperBackend_;
    int helperInitAttempts_;
};
