#pragma once

#include <QString>

class IHelper;
// manage adapter metrics (works for Windows10 only)
class AdapterMetricsController_win
{
public:
    static void updateMetrics(const QString &adapterName, IHelper *helper);
};
