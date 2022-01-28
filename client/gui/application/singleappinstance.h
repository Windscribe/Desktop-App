#ifndef SINGLE_APP_INSTANCE_H
#define SINGLE_APP_INSTANCE_H

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

Q_SIGNALS:
    void anotherInstanceRunning();

private:
    Q_DISABLE_COPY(SingleAppInstance)
    QScopedPointer< SingleAppInstancePrivate > impl;
    bool activatedRunningInstance_;
};

}

#endif // SINGLE_APP_INSTANCE_H
