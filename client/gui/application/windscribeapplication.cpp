#include "windscribeapplication.h"
#include "utils/log/categories.h"
#include <QAbstractEventDispatcher>

#ifdef Q_OS_MACOS

#include <objc/objc.h>
#include <objc/message.h>

bool dockClickHandler(id /*self*/, SEL /*_cmd*/,...)
{
    static_cast<WindscribeApplication*>(qApp)->onClickOnDock();
    return true;
}

void setupDockClickHandler()
{
    id (*performMsgSend)(id, SEL) = (id (*)(id, SEL)) objc_msgSend; // define parametered version of objc_msgSend to mimic previous runtime version's parameters

    Class cls = objc_getClass("NSApplication");
    id appInst = performMsgSend((id)cls, sel_registerName("sharedApplication"));

    if (appInst != NULL)
    {
        id delegate = performMsgSend(appInst, sel_registerName("delegate"));
        Class delClass = (Class)performMsgSend(delegate,  sel_registerName("class"));
        SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
        if (class_getInstanceMethod(delClass, shouldHandle))
        {
            class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:");
        }
        else
        {
            class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:");
        }
    }
}

#endif

WindscribeApplication::WindscribeApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    setQuitOnLastWindowClosed(false);
#ifdef Q_OS_WIN
    QAbstractEventDispatcher::instance()->installNativeEventFilter(&windowsNativeEventFilter_);
#endif

#ifdef Q_OS_MACOS
    setupDockClickHandler();
    connect(&exitHandlerMac_, &ExitHandler_mac::shouldTerminate, this, &WindscribeApplication::shouldTerminate);
#endif
}

void WindscribeApplication::onClickOnDock()
{
    emit clickOnDock();
}

bool WindscribeApplication::isExitWithRestart()
{
#ifdef Q_OS_MACOS
    return exitHandlerMac_.isExitWithRestart();
#else
    return bWasRestartOS_;
#endif
}

void WindscribeApplication::onActivateFromAnotherInstance()
{
    emit activateFromAnotherInstance();
}

bool WindscribeApplication::event(QEvent *e)
{
    if (e->type() == QEvent::Close)
        emit applicationCloseRequest();
    return QApplication::event(e);
}

void WindscribeApplication::initiateAppShutdown()
{
    bWasRestartOS_ = true;
    emit shouldTerminate();
}
