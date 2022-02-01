#ifndef MULTIPLEACCOUNTDETECTIONFACTORY_H
#define MULTIPLEACCOUNTDETECTIONFACTORY_H

#include "imultipleaccountdetection.h"

class MultipleAccountDetectionFactory
{
public:
    static IMultipleAccountDetection *create();
};

#endif // MULTIPLEACCOUNTDETECTIONFACTORY_H
