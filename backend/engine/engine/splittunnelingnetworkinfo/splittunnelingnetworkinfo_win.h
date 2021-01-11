#ifndef SPLITTUNNELINGNETWORKINFO_WIN_H
#define SPLITTUNNELINGNETWORKINFO_WIN_H

#include "splittunnelingnetworkinfo.h"

// empty at the moment, can be expanded in the future
class SplitTunnelingNetworkInfo_win : public SplitTunnelingNetworkInfo
{
public:
    SplitTunnelingNetworkInfo_win();

    void outToLog() const override;
};

#endif // SPLITTUNNELINGNETWORKINFO_WIN_H
