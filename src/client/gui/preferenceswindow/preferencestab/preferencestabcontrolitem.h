#pragma once

#include <QGraphicsObject>
#include <QTimer>
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/iconbutton.h"
#include "preferencestabtypes.h"
#include "tabbutton.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class PreferencesTabControlItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit PreferencesTabControlItem(ScalableGraphicsObject *parent,
                                       PreferencesHelper *preferencesHelper);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    PREFERENCES_TAB_TYPE currentTab();
    void setCurrentTab(PREFERENCES_TAB_TYPE tab);

    void setHeight(int newHeight);

    void setInSubpage(bool inSubpage);
    void setLoggedIn(bool loggedIn);

public slots:
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);

signals:
    void currentTabChanged(PREFERENCES_TAB_TYPE tab);
    void logoutClick();
    void loginClick();
    void quitClick();

private slots:
    void onTabClicked(PREFERENCES_TAB_TYPE tab, TabButton *button);
    void onLanguageChanged();

private:
    TabButton *generalButton_;
    TabButton *accountButton_;
    TabButton *connectionButton_;
    TabButton *robertButton_;
    TabButton *lookAndFeelButton_;
    TabButton *advancedButton_;
    TabButton *helpButton_;
    TabButton *aboutButton_;
    TabButton *logoutButton_;
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
