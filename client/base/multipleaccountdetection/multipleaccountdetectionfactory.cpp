#include "multipleaccountdetectionfactory.h"
#ifdef Q_OS_WIN
    #include "multipleaccountdetection_win.h"
#elif defined(Q_OS_MACOS) || defined (Q_OS_LINUX)
    #include "multipleaccountdetection_posix.h"
#endif

IMultipleAccountDetection *MultipleAccountDetectionFactory::create()
{
#ifdef Q_OS_WIN
    return new MultipleAccountDetection_win();
#elif defined(Q_OS_MACOS) || defined (Q_OS_LINUX)
    return new MultipleAccountDetection_posix();
#endif
}
