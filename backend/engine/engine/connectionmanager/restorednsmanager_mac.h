#ifndef RESTOREDNSMANAGER_MAC_H
#define RESTOREDNSMANAGER_MAC_H

#include <QVector>
#include <QString>
#include "Engine/Helper/ihelper.h"

class RestoreDNSManager_mac
{
public:
    RestoreDNSManager_mac(IHelper *helper);
    bool restoreState();

private:
    IHelper *helper_;
};

#endif // RESTOREDNSMANAGER_MAC_H
