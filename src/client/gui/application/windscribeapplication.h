#pragma once

#include <QApplication>

#ifdef Q_OS_WIN
    #include "windowsnativeeventfilter.h"
#endif

#ifdef Q_OS_MACOS
    #include "utils/exithandler_mac.h"
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
    void setWasRestartOSFlag() { bWasRestartOS_ = true; }
    bool isExitWithRestart();

    bool isNeedAskClose() { return bNeedAskClose_; }
    void setNeedAskClose() { bNeedAskClose_ = true; }
    void clearNeedAskClose() { bNeedAskClose_ = false; }

    void onActivateFromAnotherInstance();

signals:
    void clickOnDock();
    void activateFromAnotherInstance();
    void shouldTerminate();
    void applicationCloseRequest();

public slots:
    void initiateAppShutdown();

protected:
    bool event(QEvent *e) override;

private:
    bool bNeedAskClose_ = false;
    bool bWasRestartOS_ = false;
#ifdef Q_OS_WIN
    WindowsNativeEventFilter windowsNativeEventFilter_;
#endif

#ifdef Q_OS_MACOS
    ExitHandler_mac exitHandlerMac_;
#endif
};
