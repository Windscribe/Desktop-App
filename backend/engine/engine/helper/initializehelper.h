#ifndef INITIALIZEHELPER_H
#define INITIALIZEHELPER_H

#include <QElapsedTimer>
#include <QObject>
#include "Engine/Types/types.h"

class IHelper;
class INetworkStateManager;

// wait for initialize helper and network connectivity
class InitializeHelper : public QObject
{
    Q_OBJECT
public:
    explicit InitializeHelper(QObject *parent, IHelper *helper);
    void start();

signals:
    void finished(INIT_HELPER_RET ret);

private slots:
    void onTimerControlHelper();
    void handleHelperInit();

private:
    IHelper *helper_;
    int helperInitAttempts_;

    void printHelperVersion();
};

#endif // INITIALIZEHELPER_H
