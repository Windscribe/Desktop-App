#pragma once

#include <QGraphicsObject>
#include "preferenceswindow/preferencestab/ipreferencestabcontrol.h"
#include "connectionwindow/connectionwindowitem.h"
#include "commongraphics/resizablewindow.h"
#include "types/robertfilter.h"

// abstract interface for preferences window
class IPreferencesWindow : public ResizableWindow
{
public:
    explicit IPreferencesWindow(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
        : ResizableWindow(parent, preferences, preferencesHelper) {}
    virtual ~IPreferencesWindow() {}

    virtual PREFERENCES_TAB_TYPE currentTab() = 0;
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab) = 0;
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) = 0;

    virtual void setLoggedIn(bool loggedIn) = 0;
    virtual void setConfirmEmailResult(bool bSuccess) = 0;
    virtual void setSendLogResult(bool bSuccess) = 0;

    virtual void updateNetworkState(types::NetworkInterface network) = 0;

    virtual void addApplicationManually(QString filename) = 0;

    virtual void setPacketSizeDetectionState(bool on) = 0;
    virtual void showPacketSizeDetectionError(const QString &title, const QString &message) = 0;

    virtual void setRobertFilters(const QVector<types::RobertFilter> &filters) = 0;
    virtual void setRobertFiltersError() = 0;
    virtual void setSplitTunnelingActive(bool active) = 0;
    virtual void setWebSessionCompleted() = 0;

    virtual void onCollapse() = 0;

signals:
    virtual void signOutClick() = 0;
    virtual void loginClick() = 0;
    virtual void quitAppClick() = 0;

    virtual void viewLogClick() = 0;
    virtual void sendConfirmEmailClick() = 0;
    virtual void sendDebugLogClick() = 0;
    virtual void accountLoginClick() = 0;
    virtual void manageAccountClick() = 0;
    virtual void addEmailButtonClick() = 0;
    virtual void manageRobertRulesClick() = 0;

    virtual void currentNetworkUpdated(types::NetworkInterface) = 0;

    virtual void advancedParametersClicked() = 0;
};

Q_DECLARE_INTERFACE(IPreferencesWindow, "IPreferencesWindow")
