#pragma once

#include <QApplication>
#include <QTranslator>

#ifdef Q_OS_WIN
    #include "windowsnativeeventfilter.h"
#endif

#ifdef Q_OS_MAC
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
    void clearWasRestartOSFlag() { bWasRestartOS_ = false; }
    bool isExitWithRestart();

    bool isNeedAskClose() { return bNeedAskClose_; }
    void setNeedAskClose() { bNeedAskClose_ = true; }
    void clearNeedAskClose() { bNeedAskClose_ = false; }

    void onActivateFromAnotherInstance();
#ifdef Q_OS_WIN
    void onWinIniChanged();
#endif

signals:
    void clickOnDock();
    void activateFromAnotherInstance();
    void shouldTerminate_mac();
    void applicationCloseRequest();
#ifdef Q_OS_WIN
    void winIniChanged();
#endif

protected:
    bool event(QEvent *e) override;

private:
    bool bNeedAskClose_;
    bool bWasRestartOS_;
#ifdef Q_OS_WIN
    WindowsNativeEventFilter windowsNativeEventFilter_;
#endif

#ifdef Q_OS_MAC
    ExitHandler_mac exitHandlerMac_;
#endif
    QTranslator translator;
};
