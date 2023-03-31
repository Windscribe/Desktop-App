#ifndef PREFERENCESTABCONTROLITEM_H
#define PREFERENCESTABCONTROLITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "ipreferencestabcontrol.h"
#include "tabbutton.h"
#include "commongraphics/dividerline.h"
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

    void setInSubpage(bool inSubpage) override;
    void setLoggedIn(bool loggedIn) override;
    void updateScaling() override;

public slots:
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);

signals:
    void currentTabChanged(PREFERENCES_TAB_TYPE tab) override;
    void signOutClick() override;
    void loginClick() override;
    void quitClick() override;

private slots:
    void onTabClicked(PREFERENCES_TAB_TYPE tab, TabButton *button);
    void onLanguageChanged();

private:
    TabButton *generalButton_;
    TabButton *accountButton_;
    TabButton *connectionButton_;
    TabButton *robertButton_;
    TabButton *advancedButton_;
    TabButton *helpButton_;
    TabButton *aboutButton_;
    CommonGraphics::DividerLine *dividerLine_;
    TabButton *signOutButton_;
    TabButton *quitButton_;
    QList<TabButton *> buttonList_;

    bool loggedIn_;
    bool isExternalConfigMode_;
    int height_;

    PREFERENCES_TAB_TYPE curTab_;
    bool curInSubpage_;

    void fadeOtherButtons(TabButton *button);
    void updateBottomAnchoredButtonPos();
    void updateTopAnchoredButtonsPos();
    void clearStickySelectionOnAllTabs();
    int buttonMarginX() const;

    static constexpr int TOP_MARGIN = 8;
    static constexpr int BUTTON_MARGIN = 16;
    static constexpr int WIDTH = 2 * BUTTON_MARGIN + TabButton::BUTTON_WIDTH;
};

}

#endif // PREFERENCESTABCONTROLITEM_H
