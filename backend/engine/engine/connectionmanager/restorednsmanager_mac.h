#ifndef RESTOREDNSMANAGER_MAC_H
#define RESTOREDNSMANAGER_MAC_H

#include <QVector>
#include <QString>
#include "engine/helper/ihelper.h"

class RestoreDNSManager_mac
{
public:
    static bool restoreState(IHelper *helper);
};

#endif // RESTOREDNSMANAGER_MAC_H
