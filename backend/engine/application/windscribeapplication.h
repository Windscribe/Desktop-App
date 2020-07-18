#ifndef WINDSCRIBEAPPLICATION_H
#define WINDSCRIBEAPPLICATION_H

#include <QCoreApplication>
#include <QTranslator>

#ifdef Q_OS_WIN
    #include "windowsnativeeventfilter.h"
#endif

#ifdef Q_OS_MAC
    #include "exithandler_mac.h"
#endif

class WindscribeApplication : public QCoreApplication
{
    Q_OBJECT
public:
    explicit WindscribeApplication(int &argc, char **argv);

    static WindscribeApplication * instance()
    {
        return (WindscribeApplication *)qApp->instance();
    }

    void setWasRestartOSFlag() { bWasRestartOS_ = true; }
    void clearWasRestartOSFlag() { bWasRestartOS_ = false; }
    bool isExitWithRestart();

    bool isNeedAskClose() { return bNeedAskClose_; }
    void setNeedAskClose() { bNeedAskClose_ = true; }
    void clearNeedAskClose() { bNeedAskClose_ = false; }

signals:
    void shouldTerminate_mac();

private:
    bool bNeedAskClose_;
    bool bWasRestartOS_;
#ifdef Q_OS_WIN
    WindowsNativeEventFilter windowsNativeEventFilter_;
#endif

#ifdef Q_OS_MAC
    ExitHandler_mac exitHanlderMac_;
#endif
};

#endif // WINDSCRIBEAPPLICATION_H
