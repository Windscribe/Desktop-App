#ifndef IPREFERENCESWINDOW_H
#define IPREFERENCESWINDOW_H

#include <QGraphicsObject>
#include "preferenceswindow/preferencestab/ipreferencestabcontrol.h"
#include "ipc/generated_proto/types.pb.h"
#include "connectionwindow/connectionwindowitem.h"

// abstract interface for preferences window
class IPreferencesWindow
{
public:
    virtual ~IPreferencesWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual int recommendedHeight() = 0;
    virtual void setHeight(int height) = 0;

    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab) = 0;
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) = 0;
    virtual void setScrollBarVisibility(bool on) = 0;

    virtual void setLoggedIn(bool loggedIn) = 0;
    virtual void setConfirmEmailResult(bool bSuccess) = 0;
    virtual void setDebugLogResult(bool bSuccess) = 0;

    virtual void updateNetworkState(ProtoTypes::NetworkInterface network) = 0;

    virtual void addApplicationManually(QString filename) = 0;

    virtual void updateScaling() = 0;
    virtual void updatePageSpecific() = 0;

    virtual void setPacketSizeDetectionState(bool on) = 0;

signals:
    virtual void escape() = 0;
    virtual void helpClick() = 0;
    virtual void signOutClick() = 0;
    virtual void loginClick() = 0;
    virtual void quitAppClick() = 0;
    virtual void sizeChanged() = 0;
    virtual void resizeFinished() = 0;

    virtual void viewLogClick() = 0;
    virtual void sendConfirmEmailClick() = 0;
    virtual void sendDebugLogClick() = 0;
    virtual void noAccountLoginClick() = 0;
    virtual void clearServerRatingsClick() = 0;

    virtual void currentNetworkUpdated(ProtoTypes::NetworkInterface) = 0;

#ifdef Q_OS_WIN
    virtual void setIpv6StateInOS(bool bEnabled, bool bRestartNow) = 0;
#endif
};

Q_DECLARE_INTERFACE(IPreferencesWindow, "IPreferencesWindow")

#endif // IPREFERENCESWINDOW_H
