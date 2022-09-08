#ifndef ROUTEMONITOR_LINUX_H
#define ROUTEMONITOR_LINUX_H

#include <QThread>

class RouteMonitor_linux : public QThread
{
   Q_OBJECT
public:
    explicit RouteMonitor_linux(QObject *parent);
    ~RouteMonitor_linux();

protected:
    void run() override;

signals:
    void routesChanged();

private:
    int fd_;
};

#endif // ROUTEMONITOR_LINUX_H
