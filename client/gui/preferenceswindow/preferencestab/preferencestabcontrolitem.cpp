#include "preferencestabcontrolitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "utils/logger.h"

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
        advancedButton_,
        helpButton_,
        aboutButton_
    };

    updateTopAnchoredButtonsPos();

    dividerLine_ = new CommonGraphics::DividerLine(this, 32, 0);
    dividerLine_->setOpacity(0.1);

    signOutButton_ = new TabButton(this, TAB_UNDEFINED, "preferences/SIGN_OUT_ICON", QColor(0xff, 0xef, 0x02));
    connect(signOutButton_, &TabButton::tabClicked, this, &PreferencesTabControlItem::onTabClicked);

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
    advancedButton_->setText(tr("Advanced Options"));
    helpButton_->setText(tr("Help"));
    aboutButton_->setText(tr("About"));
    if (!loggedIn_ || isExternalConfigMode_) {
        signOutButton_->setText(tr("Login"));
    } else {
        signOutButton_->setText(tr("Log Out"));
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
    prepareGeometryChange();
    height_ = newHeight;

    updateBottomAnchoredButtonPos();
    updateTopAnchoredButtonsPos();
}

void PreferencesTabControlItem::onTabClicked(PREFERENCES_TAB_TYPE tab, TabButton *button)
{
    switch(tab) {
        case TAB_UNDEFINED:
            if (button == signOutButton_) {
                if (loggedIn_ && !isExternalConfigMode_) {
                    emit signOutClick();
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
    // May change "Sign Out" vs "Login"
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
    int signOutButtonPosY = quitButtonPosY - (BUTTON_MARGIN + TabButton::BUTTON_HEIGHT)*G_SCALE;
    int dividerPosY = signOutButtonPosY - (BUTTON_MARGIN + CommonGraphics::DividerLine::DIVIDER_HEIGHT)*G_SCALE;

    dividerLine_->setPos(0, dividerPosY);
    signOutButton_->setPos(buttonMarginX(), signOutButtonPosY);
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
