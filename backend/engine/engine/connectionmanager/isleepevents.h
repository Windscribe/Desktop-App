#ifndef ISLEEPEVENTS_H
#define ISLEEPEVENTS_H

#include <QObject>

class ISleepEvents : public QObject
{
    Q_OBJECT
public:
    explicit ISleepEvents(QObject *parent) : QObject(parent) {}
    virtual ~ISleepEvents() {}

signals:
    void gotoSleep();
    void gotoWake();
};

#endif // ISLEEPEVENTS_H
