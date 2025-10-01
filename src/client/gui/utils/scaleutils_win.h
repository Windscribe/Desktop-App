#pragma once

#include <QList>

namespace ScaleUtils_win {

QList<double> idealScaleFactors();
double closestScaleFactor(double value, QList<double> set);
double closestLargerScaleFactor(double value, QList<double> set);
double scaleFactor(double devicePixelRatio, double logicalDPI);

}
