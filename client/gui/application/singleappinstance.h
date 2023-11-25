#pragma once

#include <QObject>
#include <QScopedPointer>

namespace windscribe {

class SingleAppInstancePrivate;

class SingleAppInstance : public QObject
{
    Q_OBJECT
public:
    explicit SingleAppInstance();
    ~SingleAppInstance();

    bool activatedRunningInstance() const { return activatedRunningInstance_; }
    bool isRunning();
    void release();

signals:
    void anotherInstanceRunning();

private:
    Q_DISABLE_COPY(SingleAppInstance)
    QScopedPointer< SingleAppInstancePrivate > impl;
    bool activatedRunningInstance_;
};

}
