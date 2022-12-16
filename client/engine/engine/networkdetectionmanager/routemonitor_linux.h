#pragma once

#include <QObject>
#include <QSocketNotifier>

class RouteMonitor_linux : public QObject
{
   Q_OBJECT
public:
    explicit RouteMonitor_linux(QObject *parent = nullptr);
    ~RouteMonitor_linux();

signals:
    void routesChanged();

public slots:
    void init();
    void finish();

private slots:
    void netlinkSocketReady(QSocketDescriptor socket, QSocketNotifier::Type activationEvent);

private:
    int fd_ = -1;
    QSocketNotifier *notifier_ = nullptr;
};
