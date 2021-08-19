#ifndef IPREFERENCESTABCONTROL_H
#define IPREFERENCESTABCONTROL_H

#include <QGraphicsObject>

enum PREFERENCES_TAB_TYPE
{ TAB_GENERAL, TAB_ACCOUNT, TAB_CONNECTION, TAB_SHARE, TAB_DEBUG, TAB_UNDEFINED };

class IPreferencesTabControl
{
public:
    virtual ~IPreferencesTabControl() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual PREFERENCES_TAB_TYPE currentTab() = 0;
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab) = 0;
    virtual void setHeight(int height) = 0;
    virtual void setInSubpage(bool mode) = 0;
    virtual void setLoggedIn(bool loggedIn) = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void currentTabChanged(PREFERENCES_TAB_TYPE tab) = 0;
    virtual void helpClick() = 0;
    virtual void signOutClick() = 0;
    virtual void loginClick() = 0;
    virtual void quitClick() = 0;

};

Q_DECLARE_INTERFACE(IPreferencesTabControl, "IPreferencesTabControl")


#endif // IPREFERENCESTABCONTROL_H
