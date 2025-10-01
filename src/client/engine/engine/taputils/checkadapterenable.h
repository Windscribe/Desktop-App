#pragma once

#include <QString>
#include "engine/helper/helper.h"

class CheckAdapterEnable
{
public:
    static bool isAdapterDisabled(Helper *helper, const QString &adapterName);
};
