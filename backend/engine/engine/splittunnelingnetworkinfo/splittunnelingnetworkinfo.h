#ifndef SPLITTUNNELINGNETWORKINFO_H
#define SPLITTUNNELINGNETWORKINFO_H

#include <QStringList>
#include "engine/types/protocoltype.h"

// Keep network info which need send to helper for split tunneling (base class)
class SplitTunnelingNetworkInfo
{
public:
    virtual ~SplitTunnelingNetworkInfo() {}
    virtual void outToLog() const = 0;

    static SplitTunnelingNetworkInfo *createObject();
};

#endif // SPLITTUNNELINGNETWORKINFO_H
