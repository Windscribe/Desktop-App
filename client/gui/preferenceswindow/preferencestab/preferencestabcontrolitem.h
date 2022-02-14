#ifndef PREFERENCESTABCONTROLITEM_H
#define PREFERENCESTABCONTROLITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "ipreferencestabcontrol.h"
#include "commongraphics/iconbutton.h"
#include "tooltips/tooltiptypes.h"
#include "backend/preferences/preferenceshelper.h"

namespace PreferencesWindow {


class PreferencesTabControlItem : public ScalableGraphicsObject, public IPreferencesTabControl
{
    Q_OBJECT
    Q_INTERFACES(IPreferencesTabControl)
public:
    explicit PreferencesTabControlItem(ScalableGraphicsObject *parent,
                                       PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override;

    PREFERENCES_TAB_TYPE currentTab() override;
    void setCurrentTab(PREFERENCES_TAB_TYPE tab) override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setHeight(int newHeight) override;

    void changeToGeneral();
    void changeToAccount();
    void changeToConnection();
    void changeToShare();
    void changeToDebug();

    void setInSubpage(bool inSubpage) override;
    void setLoggedIn(bool loggedIn) override;
    void updateScaling() override;

public slots:
    void onButtonHoverLeave();
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);

signals:
    void currentTabChanged(PREFERENCES_TAB_TYPE tab) override;
    void helpClick() override;
    void signOutClick() override;
    void loginClick() override;
    void quitClick() override;

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
    void updateLinePos();

    static constexpr int WIDTH = 48; //  16 + 16 + 16;
    int height_;

    int buttonMarginX() const;

    static constexpr int SEPERATOR_Y = 16;
    static constexpr int TOP_BUTTON_Y = 16;

    static constexpr int BUTTON_WIDTH = WIDTH - 4;
    static constexpr int BUTTON_HEIGHT = 16;

    // Keep tooltip 2px to the right of the pic: 10 = 16 / 2 + 2.
    static constexpr int SCENE_OFFSET_X = BUTTON_WIDTH / 2 + 10;
    static constexpr int SCENE_OFFSET_Y = 7;

    void clearStickySelectionOnAllTabs();
};

}

#endif // PREFERENCESTABCONTROLITEM_H
