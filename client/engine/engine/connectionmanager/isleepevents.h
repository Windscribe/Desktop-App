#pragma once

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
