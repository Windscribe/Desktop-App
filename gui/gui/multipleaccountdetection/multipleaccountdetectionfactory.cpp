#include "multipleaccountdetectionfactory.h"
#ifdef Q_OS_WIN
    #include "multipleaccountdetection_win.h"
#elif defined Q_OS_MAC
    #include "multipleaccountdetection_mac.h"
#endif

IMultipleAccountDetection *MultipleAccountDetectionFactory::create()
{
#ifdef Q_OS_WIN
    return new MultipleAccountDetection_win();
#elif defined Q_OS_MAC
    return new MultipleAccountDetection_mac();
#endif
}
