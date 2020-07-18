#ifndef ADAPTERMETRICSCONTROLLER_WIN_H
#define ADAPTERMETRICSCONTROLLER_WIN_H

#include <QString>

class IHelper;
// manage adapter metrics (works for Windows10 only)
class AdapterMetricsController_win
{
public:
    static void updateMetrics(const QString &tapName, IHelper *helper);
};

#endif // ADAPTERMETRICSCONTROLLER_WIN_H
