#include "windscribeapplication.h"
#include "Utils/logger.h"
#include <QAbstractEventDispatcher>

#ifdef Q_OS_MAC

#include <objc/objc.h>
#include <objc/message.h>

bool dockClickHandler(id self, SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
   ((WindscribeApplication*)qApp)->onClickOnDock();
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

bool focusLossHandler(id self, SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
   ((WindscribeApplication*)qApp)->onFocusLoss();
    return true;
}

void setupFocusLossHandler()
{
    id (*performMsgSend)(id, SEL) = (id (*)(id, SEL)) objc_msgSend; // define parametered version of objc_msgSend to mimic previous runtime version's parameters

    Class cls = objc_getClass("NSApplication");
    id appInst = performMsgSend((id)cls, sel_registerName("sharedApplication"));

    if (appInst != NULL)
    {
        id delegate = performMsgSend(appInst, sel_registerName("delegate"));
        Class delClass = (Class)performMsgSend(delegate,  sel_registerName("class"));

        SEL methodSelector = sel_registerName("applicationDidResignActive:");
        Method mtdFunc = class_getInstanceMethod(delClass, methodSelector);
        const char *types = method_getTypeEncoding(mtdFunc);

        if (class_getInstanceMethod(delClass, methodSelector))
        {
            class_replaceMethod(delClass, methodSelector, (IMP)focusLossHandler, types);
        }
        else
        {
            class_addMethod(delClass, methodSelector, (IMP)focusLossHandler, types);
        }
    }
}

#endif

WindscribeApplication::WindscribeApplication(int &argc, char **argv) : QApplication(argc, argv),
    bNeedAskClose_(false), bWasRestartOS_(false)
{
#ifdef Q_OS_WIN
    QAbstractEventDispatcher::instance()->installNativeEventFilter(&windowsNativeEventFilter_);
#endif

#ifdef Q_OS_MAC
    setupDockClickHandler();
    setupFocusLossHandler();
    connect(&exitHanlderMac_, SIGNAL(shouldTerminate()), SIGNAL(shouldTerminate_mac()));
    connect(&openLocationsHandlerMac_, SIGNAL(receivedOpenLocationsMessage()), SIGNAL(receivedOpenLocationsMessage()));
#endif
}

void WindscribeApplication::onClickOnDock()
{
    emit clickOnDock();
}

void WindscribeApplication::onFocusLoss()
{
#ifdef Q_OS_MAC
    openLocationsHandlerMac_.unsuspendObservers();
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

QString WindscribeApplication::changeLanguage(const QString &lang)
{
    if (lang == "en")
    {
        removeTranslator(&translator);
        qCDebug(LOG_BASIC) << "Language changed:" << lang;
        return lang;
    }
    else
    {
#if defined Q_OS_WIN
        QString filename = QApplication::applicationDirPath() + "/languages/" + lang + ".qm";
#elif defined Q_OS_MAC
        QString filename = QApplication::applicationDirPath() + "/../Languages/" + lang + ".qm";
#endif
        if (translator.load(filename))
        {
            qCDebug(LOG_BASIC) << "Language changed:" << lang;
            installTranslator(&translator);
            return lang;
        }
        else
        {
            qCDebug(LOG_BASIC) << "Failed load language file for" << lang;
            // set English on error
            removeTranslator(&translator);
            qCDebug(LOG_BASIC) << "Language changed: en";
            return "en";
        }
    }
}

void WindscribeApplication::onActivateFromAnotherInstance()
{
    emit activateFromAnotherInstance();
}

void WindscribeApplication::onOpenLocationsFromAnotherInstance()
{
    emit openLocationsFromAnotherInstance();
}
