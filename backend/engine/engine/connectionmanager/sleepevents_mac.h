#ifndef SLEEPEVENTS_MAC_H
#define SLEEPEVENTS_MAC_H

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

#endif // SLEEPEVENTS_MAC_H
