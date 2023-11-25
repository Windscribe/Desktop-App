#pragma once

#include <QVector>
#include <QString>
#include "engine/helper/ihelper.h"

class RestoreDNSManager_mac
{
public:
    static bool restoreState(IHelper *helper);
};
