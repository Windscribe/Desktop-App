#include "splittunnelingnetworkinfo.h"

#ifdef Q_OS_WIN
    #include "splittunnelingnetworkinfo_win.h"
#else
    #include "splittunnelingnetworkinfo_mac.h"
#endif


SplitTunnelingNetworkInfo *SplitTunnelingNetworkInfo::createObject()
{
#ifdef Q_OS_WIN
    return new SplitTunnelingNetworkInfo_win();
#else
    return new SplitTunnelingNetworkInfo_mac();
#endif
}
