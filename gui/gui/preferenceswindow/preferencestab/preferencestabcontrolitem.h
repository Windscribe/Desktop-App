#ifndef PREFERENCESTABCONTROLITEM_H
#define PREFERENCESTABCONTROLITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "ipreferencestabcontrol.h"
#include "commongraphics/iconbutton.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {


class PreferencesTabControlItem : public ScalableGraphicsObject, public IPreferencesTabControl
{
    Q_OBJECT
    Q_INTERFACES(IPreferencesTabControl)
public:
    explicit PreferencesTabControlItem(ScalableGraphicsObject *parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject();

    virtual PREFERENCES_TAB_TYPE currentTab();
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void setHeight(int newHeight);

    void changeToGeneral();
    void changeToAccount();
    void changeToConnection();
    void changeToShare();
    void changeToDebug();

    virtual void setInSubpage(bool inSubpage);
    virtual void setLoggedIn(bool loggedIn);
    virtual void updateScaling();

public slots:
    void onButtonHoverLeave();

signals:
    void currentTabChanged(PREFERENCES_TAB_TYPE tab);
    void helpClick();
    void signOutClick();
    void loginClick();
    void quitClick();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onGeneralButtonClicked();
    void onAccountButtonClicked();
    void onConnectionButtonClicked();
    void onShareButtonClicked();
    void onDebugButtonClicked();

    void onHelpButtonClicked();
    void onSignOutButtonClicked();
    void onQuitButtonClicked();

    void onLinePosChanged(const QVariant &value);

    void onGeneralHoverEnter();
    void onAccountHoverEnter();
    void onConnectionHoverEnter();
    void onShareHoverEnter();
    void onDebugHoverEnter();
    void onHelpHoverEnter();
    void onSignOutHoverEnter();
    void onQuitHoverEnter();
    void onAbstractButtonHoverEnter(QGraphicsObject *button, QString text);

private:
    IconButton *generalButton_;
    IconButton *accountButton_;
    IconButton *connectionButton_;
    IconButton *shareButton_;
    IconButton *debugButton_;

    IconButton *helpButton_;
    IconButton *signOutButton_;
    IconButton *quitButton_;

    bool loggedIn_;
    int curLineStartPos_;
    QVariantAnimation linePosAnimation_;

    PREFERENCES_TAB_TYPE curTab_;
    bool curInSubpage_;

    void fadeButtons(double newOpacity, int animationSpeed);

    void updateBottomAnchoredButtonPos();
    void updateTopAnchoredButtonsPos();

    const int WIDTH = 48; //  16 + 16 + 16;
    int height_;

    int buttonMarginX() const;

    const int SEPERATOR_Y = 16;
    const int GENERAL_BUTTON_Y = 16;
    const int ACCOUNT_BUTTON_Y = GENERAL_BUTTON_Y + 16 + SEPERATOR_Y;
    const int CONNECTION_BUTTON_Y = ACCOUNT_BUTTON_Y + 16 + SEPERATOR_Y;
    const int SHARE_BUTTON_Y = CONNECTION_BUTTON_Y + 16 + SEPERATOR_Y;
    const int DEBUG_BUTTON_Y = SHARE_BUTTON_Y + 16 + SEPERATOR_Y;
    const int BUTTON_WIDTH = WIDTH - 4;

    // Keep tooltip 2px to the right of the pic: 10 = 16 / 2 + 2.
    const int SCENE_OFFSET_X = BUTTON_WIDTH / 2 + 10;
    const int SCENE_OFFSET_Y = 7;

    void clearStickySelectionOnAllTabs();
};

}

#endif // PREFERENCESTABCONTROLITEM_H
