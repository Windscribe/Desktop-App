#pragma once

#include "imultipleaccountdetection.h"

class MultipleAccountDetectionFactory
{
public:
    static IMultipleAccountDetection *create();
};
