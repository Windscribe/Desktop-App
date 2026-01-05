#include "preferencestabcontrolitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "dpiscalemanager.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

PreferencesTabControlItem::PreferencesTabControlItem(ScalableGraphicsObject * parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), loggedIn_(false), isExternalConfigMode_(preferencesHelper->isExternalConfigMode()),
      curTab_(TAB_UNDEFINED), curInSubpage_(false)
{
    height_ = TOP_MARGIN + 9 * (BUTTON_MARGIN + TabButton::BUTTON_HEIGHT);

    generalButton_ = new TabButton(this, TAB_GENERAL, "preferences/GENERAL_ICON");
    connect(generalButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    accountButton_ = new TabButton(this, TAB_ACCOUNT, "preferences/ACCOUNT_ICON");
    connect(accountButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    connectionButton_ = new TabButton(this, TAB_CONNECTION, "preferences/CONNECTION_ICON");
    connect(connectionButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    robertButton_ = new TabButton(this, TAB_ROBERT, "preferences/ROBERT_ICON");
    connect(robertButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    lookAndFeelButton_ = new TabButton(this, TAB_LOOK_AND_FEEL, "preferences/LOOK_AND_FEEL_ICON");
    connect(lookAndFeelButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    advancedButton_ = new TabButton(this, TAB_ADVANCED, "preferences/ADVANCED_ICON");
    connect(advancedButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    helpButton_ = new TabButton(this, TAB_HELP, "preferences/HELP_ICON");
    connect(helpButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    aboutButton_ = new TabButton(this, TAB_ABOUT, "preferences/ABOUT_ICON");
    connect(aboutButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    // make a list of top anchored buttons for convenience
    buttonList_ = {
        generalButton_,
        accountButton_,
        connectionButton_,
        robertButton_,
        lookAndFeelButton_,
        advancedButton_,
        helpButton_,
        aboutButton_
    };

    updateTopAnchoredButtonsPos();

    logoutButton_ = new TabButton(this, TAB_UNDEFINED, "preferences/LOGOUT_ICON", QColor(0xff, 0xef, 0x02));
    connect(logoutButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    quitButton_ = new TabButton(this, TAB_UNDEFINED, "preferences/QUIT_ICON", QColor(0xff, 0x3b, 0x3b));
    connect(quitButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

    updateBottomAnchoredButtonPos();

    connect(preferencesHelper, &PreferencesHelper::isExternalConfigModeChanged, this, &PreferencesTabControlItem::onIsExternalConfigModeChanged);

    //  init general tab on start without animation
    generalButton_->setStickySelection(true);
    generalButton_->setSelected(true);
    curTab_ = TAB_GENERAL;

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &PreferencesTabControlItem::onLanguageChanged);
    onLanguageChanged();
}

void PreferencesTabControlItem::onLanguageChanged()
{
    generalButton_->setText(tr("General"));
    accountButton_->setText(tr("Account"));
    connectionButton_->setText(tr("Connection"));
    robertButton_->setText(tr("R.O.B.E.R.T."));
    lookAndFeelButton_->setText(tr("Look & Feel"));
    advancedButton_->setText(tr("Advanced Options"));
    helpButton_->setText(tr("Help"));
    aboutButton_->setText(tr("About"));
    if (!loggedIn_ || isExternalConfigMode_) {
        logoutButton_->setText(tr("Login"));
    } else {
        logoutButton_->setText(tr("Log Out"));
    }
    quitButton_->setText(tr("Quit"));
}

PREFERENCES_TAB_TYPE PreferencesTabControlItem::currentTab()
{
    return curTab_;
}

void PreferencesTabControlItem::setCurrentTab(PREFERENCES_TAB_TYPE tab)
{
    for (TabButton *btn : buttonList_) {
        if (btn->tab() == tab) {
            fadeOtherButtons(btn);
            btn->setStickySelection(true);
            btn->setSelected(true);
            curTab_ = tab;
            return;
        }
    }
}

QRectF PreferencesTabControlItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, height_);
}

void PreferencesTabControlItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void PreferencesTabControlItem::setHeight(int newHeight)
{
    if (height_ != newHeight) {
        prepareGeometryChange();
        height_ = newHeight;

        updateBottomAnchoredButtonPos();
        updateTopAnchoredButtonsPos();
    }
}

void PreferencesTabControlItem::onTabClicked(PREFERENCES_TAB_TYPE tab, TabButton *button)
{
    switch(tab) {
        case TAB_UNDEFINED:
            if (button == logoutButton_) {
                if (loggedIn_ && !isExternalConfigMode_) {
                    emit logoutClick();
                } else {
                    emit loginClick();
                }
            } else if (button == quitButton_) {
                emit quitClick();
            }
            break;
        default:
            if (curTab_ != tab || curInSubpage_) {
                fadeOtherButtons(button);
                button->setStickySelection(true);
                button->setSelected(true);

                curTab_ = tab;
                emit currentTabChanged(tab);
            }
            break;
    }
}

void PreferencesTabControlItem::setInSubpage(bool inSubpage)
{
    curInSubpage_ = inSubpage;
}

void PreferencesTabControlItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
    onLanguageChanged();
}

void PreferencesTabControlItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    updateTopAnchoredButtonsPos();
    updateBottomAnchoredButtonPos();
}

void PreferencesTabControlItem::onIsExternalConfigModeChanged(bool bIsExternalConfigMode)
{
    isExternalConfigMode_ = bIsExternalConfigMode;
    // May change "Log Out" vs "Login"
    onLanguageChanged();

    accountButton_->setVisible(!bIsExternalConfigMode);
    robertButton_->setVisible(!bIsExternalConfigMode);
    if (bIsExternalConfigMode && curTab_ == TAB_ACCOUNT) {
        onTabClicked(TAB_GENERAL, generalButton_);
    }
    updateTopAnchoredButtonsPos();
}

void PreferencesTabControlItem::fadeOtherButtons(TabButton *button)
{
    for (TabButton *btn : buttonList_) {
        if (btn != button) {
            btn->setStickySelection(false);
            btn->setSelected(false);
        }
    }
}

void PreferencesTabControlItem::updateBottomAnchoredButtonPos()
{
    int quitButtonPosY = height_ - (BUTTON_MARGIN + TabButton::BUTTON_HEIGHT)*G_SCALE;
    int logoutButtonPosY = quitButtonPosY - (BUTTON_MARGIN + TabButton::BUTTON_HEIGHT)*G_SCALE;

    logoutButton_->setPos(buttonMarginX(), logoutButtonPosY);
    quitButton_->setPos(buttonMarginX(), quitButtonPosY);
}

void PreferencesTabControlItem::updateTopAnchoredButtonsPos()
{
    int yPos = TOP_MARGIN + BUTTON_MARGIN;

    for (TabButton *btn : buttonList_) {
        if (btn->isVisible()) {
            btn->setPos(buttonMarginX(), yPos*G_SCALE);
            yPos += BUTTON_MARGIN + TabButton::BUTTON_HEIGHT;
        }
    }
}

int PreferencesTabControlItem::buttonMarginX() const
{
    return (WIDTH - TabButton::BUTTON_WIDTH) / 2 * G_SCALE;
}

}
