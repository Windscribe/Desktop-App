#pragma once

#include <QString>
#include "engine/helper/helper.h"

// manage adapter metrics (works for Windows10 only)
class AdapterMetricsController_win
{
public:
    static void updateMetrics(const QString &adapterName, Helper *helper);
};
