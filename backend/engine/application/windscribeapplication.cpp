#include "windscribeapplication.h"
#include "Utils/logger.h"
#include <QAbstractEventDispatcher>

#ifdef Q_OS_MAC

#include <objc/objc.h>
#include <objc/message.h>

//bool dockClickHandler(id self, SEL _cmd,...)
//{
//    Q_UNUSED(self)
//    Q_UNUSED(_cmd)
//   ((WindscribeApplication*)qApp)->onClickOnDock();
//     return true;
//}

//void setupDockClickHandler()
//{
//    Class cls = objc_getClass("NSApplication");
//    id appInst = objc_msgSend((id)cls, sel_registerName("sharedApplication"));

//    if (appInst != NULL)
//    {
//        id delegate = objc_msgSend(appInst, sel_registerName("delegate"));
//        Class delClass = (Class)objc_msgSend(delegate,  sel_registerName("class"));
//        SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
//        if (class_getInstanceMethod(delClass, shouldHandle))
//        {
//            class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:");
//        }
//        else
//        {
//            class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:");
//        }
//    }
//}

#endif

WindscribeApplication::WindscribeApplication(int &argc, char **argv) : QCoreApplication(argc, argv),
    bNeedAskClose_(false), bWasRestartOS_(false)
{
#ifdef Q_OS_WIN
    QAbstractEventDispatcher::instance()->installNativeEventFilter(&windowsNativeEventFilter_);
#endif

#ifdef Q_OS_MAC
    //setupDockClickHandler();
    connect(&exitHanlderMac_, SIGNAL(shouldTerminate()), SIGNAL(shouldTerminate_mac()));
#endif
}

bool WindscribeApplication::isExitWithRestart()
{
#ifdef Q_OS_MAC
    return exitHanlderMac_.isExitWithRestart();
#else
    return bWasRestartOS_;
#endif
}

