#include "scaleutils_win.h"

namespace ScaleUtils_win {

QList<double> idealScaleFactors()
{
    QList<double> scaleFactors;
    scaleFactors << 0.5 << 1.0 << 1.5 << 2.0 << 2.5 << 3.0;
    // scaleFactors << 0.5 << 0.75 << 1.0 << 1.25 << 1.5 << 1.75 << 2.0 << 2.25 << 2.5 << 2.75 << 3.0; // not as well tested
    return scaleFactors;
}

double scaleFactor(double devicePixelRatio, double logicalDPI)
{
    QString scaleFactorStr = "";
    double scaleFactorDouble = 1.0;
    double scaleFactorOG = 1.0;

    // Scaling for non-(100/125%)
    if (devicePixelRatio > 3) // 350%
    {
        scaleFactorOG = logicalDPI/2 / 100;
        scaleFactorDouble = closestScaleFactor(scaleFactorOG, idealScaleFactors());
    }
    else if (devicePixelRatio > 2) // 250 & 300%
    {
        qreal factor = logicalDPI*(static_cast<qreal>(5)/8); // 300%

        if (logicalDPI < 90) // 250%
        {
            factor = logicalDPI*(static_cast<qreal>(2)/3);
        }

        scaleFactorOG = factor / 100;
        scaleFactorDouble = closestScaleFactor(scaleFactorOG, idealScaleFactors());
    }
    else if (devicePixelRatio > 1) // 150 -> 225%
    {
        scaleFactorOG = logicalDPI / 100;
        scaleFactorDouble = closestScaleFactor(scaleFactorOG, idealScaleFactors());
    }

    return scaleFactorDouble;
}

double closestScaleFactor(double value, QList<double> set)
{
    double closest = 1.0;
    double closestDiff = fabs(value - closest);

    for (double scale_factor : std::as_const(set))
    {
        double diff = fabs(value - scale_factor);
        if (diff < closestDiff)
        {
            closest = scale_factor;
            closestDiff = diff;
        }
    }

    return closest;
}

double closestLargerScaleFactor(double value, QList<double> set)
{
    double closest = 1.0;
    double closestDiff = 99999;

    for (double scale_factor : std::as_const(set))
    {
        if (value < scale_factor)
        {
            double diff = fabs(value - scale_factor);
            if (diff < closestDiff)
            {
                closest = scale_factor;
                closestDiff = diff;
            }
        }
    }

    return closest;
}



}
