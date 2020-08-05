#ifndef WINDSCRIBEAPPLICATION_H
#define WINDSCRIBEAPPLICATION_H

#include <QApplication>
#include <QTranslator>

#ifdef Q_OS_WIN
    #include "windowsnativeeventfilter.h"
#endif

#ifdef Q_OS_MAC
    #include "exithandler_mac.h"
    #include "openlocationshandler_mac.h"
#endif

class WindscribeApplication : public QApplication
{
    Q_OBJECT
public:
    explicit WindscribeApplication(int &argc, char **argv);

    static WindscribeApplication *instance()
    {
        return static_cast<WindscribeApplication *>(qApp->instance());
    }

    void onClickOnDock();
    void onFocusLoss();
    void setWasRestartOSFlag() { bWasRestartOS_ = true; }
    void clearWasRestartOSFlag() { bWasRestartOS_ = false; }
    bool isExitWithRestart();
    QString changeLanguage(const QString &lang);

    bool isNeedAskClose() { return bNeedAskClose_; }
    void setNeedAskClose() { bNeedAskClose_ = true; }
    void clearNeedAskClose() { bNeedAskClose_ = false; }

    void onActivateFromAnotherInstance();
    void onOpenLocationsFromAnotherInstance();

signals:
    void clickOnDock();
    void activateFromAnotherInstance();
    void openLocationsFromAnotherInstance();
    void shouldTerminate_mac();
    void receivedOpenLocationsMessage();

private:
    bool bNeedAskClose_;
    bool bWasRestartOS_;
#ifdef Q_OS_WIN
    WindowsNativeEventFilter windowsNativeEventFilter_;
#endif

#ifdef Q_OS_MAC
    ExitHandler_mac exitHanlderMac_;
    OpenLocationsHandler_mac openLocationsHandlerMac_;
#endif
    QTranslator translator;
};

#endif // WINDSCRIBEAPPLICATION_H
