#pragma once

#include <QObject>
#include <QThread>

class NetworkChangeWorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit NetworkChangeWorkerThread(QObject *parent = nullptr);
    virtual ~NetworkChangeWorkerThread();

    void earlyExit();

    void SignalIpInterfaceChange() const;

signals:
    void networkChanged();

public slots:
    void run() override;

private:
    void *hExitEvent_;
    void *hIpInterfaceChange_;
};
