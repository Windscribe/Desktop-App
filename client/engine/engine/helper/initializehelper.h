#ifndef INITIALIZEHELPER_H
#define INITIALIZEHELPER_H

#include <QElapsedTimer>
#include <QObject>
#include "engine/types/types.h"

class IHelper;

// wait for initialize helper
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
