#pragma once

#include <QObject>
#include "isleepevents.h"

class SleepEvents_mac : public ISleepEvents
{
    Q_OBJECT
public:
    explicit SleepEvents_mac(QObject *parent = 0);
    virtual ~SleepEvents_mac();

    virtual void emitGotoSleep();
    virtual void emitGotoWake();
};
