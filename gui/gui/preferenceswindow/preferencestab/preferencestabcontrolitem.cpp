#include "preferencestabcontrolitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

PreferencesTabControlItem::PreferencesTabControlItem(ScalableGraphicsObject * parent) : ScalableGraphicsObject(parent)
    , loggedIn_(false)
    , curTab_(TAB_UNDEFINED)
{
    height_ = 303;
    curLineStartPos_ = GENERAL_BUTTON_Y;

    generalButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/GENERAL_ICON", this);

    generalButton_->setStickySelection(true);
    connect(generalButton_, SIGNAL(clicked()), SLOT(onGeneralButtonClicked()));
    connect(generalButton_, SIGNAL(hoverEnter()), SLOT(onGeneralHoverEnter()));
    connect(generalButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    accountButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/ACCOUNT_ICON", this);
    connect(accountButton_, SIGNAL(clicked()), SLOT(onAccountButtonClicked()));
    connect(accountButton_, SIGNAL(hoverEnter()), SLOT(onAccountHoverEnter()));
    connect(accountButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    connectionButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/CONNECTION_ICON", this);
    connect(connectionButton_, SIGNAL(clicked()), SLOT(onConnectionButtonClicked()));
    connect(connectionButton_, SIGNAL(hoverEnter()), SLOT(onConnectionHoverEnter()));
    connect(connectionButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    shareButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/SHARING_ICON", this);
    connect(shareButton_, SIGNAL(clicked()), SLOT(onShareButtonClicked()));
    connect(shareButton_, SIGNAL(hoverEnter()), SLOT(onShareHoverEnter()));
    connect(shareButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    debugButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/DEBUG_ICON", this);
    connect(debugButton_, SIGNAL(clicked()), SLOT(onDebugButtonClicked()));
    connect(debugButton_, SIGNAL(hoverEnter()), SLOT(onDebugHoverEnter()));
    connect(debugButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    updateTopAnchoredButtonsPos();

    helpButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/HELP_ICON", this, OPACITY_FULL);
    signOutButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/SIGN_OUT_ICON", this, OPACITY_FULL);
    quitButton_ = new IconButton(BUTTON_WIDTH,16, "preferences/QUIT_ICON", this, OPACITY_FULL);

    connect(helpButton_, SIGNAL(clicked()), SLOT(onHelpButtonClicked()));
    connect(helpButton_, SIGNAL(hoverEnter()), SLOT(onHelpHoverEnter()));
    connect(helpButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    connect(signOutButton_, SIGNAL(clicked()), SLOT(onSignOutButtonClicked()));
    connect(signOutButton_, SIGNAL(hoverEnter()), SLOT(onSignOutHoverEnter()));
    connect(signOutButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    connect(quitButton_, SIGNAL(clicked()), SLOT(onQuitButtonClicked()));
    connect(quitButton_, SIGNAL(hoverEnter()), SLOT(onQuitHoverEnter()));
    connect(quitButton_, SIGNAL(hoverLeave()), SLOT(onButtonHoverLeave()));

    updateBottomAnchoredButtonPos();

    connect(&linePosAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onLinePosChanged(QVariant)));

    //  init general tab on start without animation
    generalButton_->setSelected(true);
    curTab_ = TAB_GENERAL;
}

QGraphicsObject *PreferencesTabControlItem::getGraphicsObject()
{
    return this;
}

PREFERENCES_TAB_TYPE PreferencesTabControlItem::currentTab()
{
    return curTab_;
}

void PreferencesTabControlItem::setCurrentTab(PREFERENCES_TAB_TYPE tab)
{
    if (tab == TAB_GENERAL)        { changeToGeneral()   ;}
    else if (tab == TAB_ACCOUNT)   { changeToAccount()   ;}
    else if (tab == TAB_CONNECTION){ changeToConnection();}
    else if (tab == TAB_SHARE)     { changeToShare()     ;}
    else if (tab == TAB_DEBUG)     { changeToDebug()     ;}
}

QRectF PreferencesTabControlItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, height_);
}

void PreferencesTabControlItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // Line
    painter->setOpacity(initOpacity);
    painter->fillRect(QRect(0, curLineStartPos_ * G_SCALE, 2*G_SCALE, 16*G_SCALE), QBrush(Qt::white));
}

void PreferencesTabControlItem::setHeight(int newHeight)
{
    prepareGeometryChange();
    height_ = newHeight;

    updateBottomAnchoredButtonPos();
    updateTopAnchoredButtonsPos();
}

void PreferencesTabControlItem::changeToGeneral()
{
    if (curTab_ != TAB_GENERAL)
    {
        clearStickySelectionOnAllTabs();
        fadeButtons(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
        generalButton_->setSelected(true);
        generalButton_->setStickySelection(true);
        generalButton_->animateOpacityChange(OPACITY_FULL, ANIMATION_SPEED_FAST);

        startAnAnimation(linePosAnimation_, curLineStartPos_, GENERAL_BUTTON_Y, ANIMATION_SPEED_FAST);

        curTab_ = TAB_GENERAL;
        emit currentTabChanged(TAB_GENERAL);
    }
}

void PreferencesTabControlItem::changeToAccount()
{
    if (curTab_ != TAB_ACCOUNT)
    {
        clearStickySelectionOnAllTabs();
        fadeButtons(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
        accountButton_->setSelected(true);
        accountButton_->setStickySelection(true);
        accountButton_->animateOpacityChange(OPACITY_FULL, ANIMATION_SPEED_FAST);

        startAnAnimation(linePosAnimation_, curLineStartPos_, ACCOUNT_BUTTON_Y, ANIMATION_SPEED_FAST);

        curTab_ = TAB_ACCOUNT;
        emit currentTabChanged(TAB_ACCOUNT);
    }
}

void PreferencesTabControlItem::changeToConnection()
{
    if (curTab_ != TAB_CONNECTION)
    {
        clearStickySelectionOnAllTabs();
        fadeButtons(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
        connectionButton_->setSelected(true);
        connectionButton_->setStickySelection(true);
        connectionButton_->animateOpacityChange(OPACITY_FULL, ANIMATION_SPEED_FAST);

        startAnAnimation(linePosAnimation_, curLineStartPos_, CONNECTION_BUTTON_Y, ANIMATION_SPEED_FAST);

        curTab_ = TAB_CONNECTION;
        emit currentTabChanged(TAB_CONNECTION);
    }
}

void PreferencesTabControlItem::changeToShare()
{
    if (curTab_ != TAB_SHARE)
    {
        clearStickySelectionOnAllTabs();
        fadeButtons(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
        shareButton_->setSelected(true);
        shareButton_->setStickySelection(true);
        shareButton_->animateOpacityChange(OPACITY_FULL, ANIMATION_SPEED_FAST);

        startAnAnimation(linePosAnimation_, curLineStartPos_, SHARE_BUTTON_Y, ANIMATION_SPEED_FAST);

        curTab_ = TAB_SHARE;
        emit currentTabChanged(TAB_SHARE);
    }
}

void PreferencesTabControlItem::changeToDebug()
{
    if (curTab_ != TAB_DEBUG)
    {
        clearStickySelectionOnAllTabs();
        fadeButtons(OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);
        debugButton_->setSelected(true);
        debugButton_->setStickySelection(true);
        debugButton_->animateOpacityChange(OPACITY_FULL, ANIMATION_SPEED_FAST);

        startAnAnimation(linePosAnimation_, curLineStartPos_, DEBUG_BUTTON_Y, ANIMATION_SPEED_FAST);

        curTab_ = TAB_DEBUG;
        emit currentTabChanged(TAB_DEBUG);
    }
}

void PreferencesTabControlItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
}

void PreferencesTabControlItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    updateTopAnchoredButtonsPos();
    updateBottomAnchoredButtonPos();
}

void PreferencesTabControlItem::onGeneralButtonClicked()
{
    changeToGeneral();
}

void PreferencesTabControlItem::onAccountButtonClicked()
{
    changeToAccount();
}

void PreferencesTabControlItem::onConnectionButtonClicked()
{
    changeToConnection();
}

void PreferencesTabControlItem::onShareButtonClicked()
{
    changeToShare();
}

void PreferencesTabControlItem::onDebugButtonClicked()
{
    changeToDebug();
}

void PreferencesTabControlItem::onHelpButtonClicked()
{
    emit helpClick();
}

void PreferencesTabControlItem::onSignOutButtonClicked()
{
    if (loggedIn_)
    {
        emit signOutClick();
    }
    else
    {
        emit loginClick();
    }
}

void PreferencesTabControlItem::onQuitButtonClicked()
{
    emit quitClick();
}

void PreferencesTabControlItem::onLinePosChanged(const QVariant &value)
{
    curLineStartPos_ = value.toInt();
    update();
}

void PreferencesTabControlItem::onGeneralHoverEnter()
{   
    onAbstractButtonHoverEnter(generalButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "General"));
}

void PreferencesTabControlItem::onAccountHoverEnter()
{
    onAbstractButtonHoverEnter(accountButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Account"));
}

void PreferencesTabControlItem::onConnectionHoverEnter()
{
    onAbstractButtonHoverEnter(connectionButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Connection"));
}

void PreferencesTabControlItem::onShareHoverEnter()
{
    onAbstractButtonHoverEnter(shareButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Share"));
}

void PreferencesTabControlItem::onDebugHoverEnter()
{
    onAbstractButtonHoverEnter(debugButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Debug"));
}

void PreferencesTabControlItem::onHelpHoverEnter()
{
    onAbstractButtonHoverEnter(helpButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Help"));
}

void PreferencesTabControlItem::onSignOutHoverEnter()
{
    if (loggedIn_)
    {
        onAbstractButtonHoverEnter(signOutButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Sign Out"));
    }
    else
    {
        onAbstractButtonHoverEnter(signOutButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Login"));
    }
}

void PreferencesTabControlItem::onQuitHoverEnter()
{
    onAbstractButtonHoverEnter(quitButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Quit"));
}

void PreferencesTabControlItem::onAbstractButtonHoverEnter(QGraphicsObject *button, QString text )
{
    if (button != nullptr)
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(button->scenePos()));

        int posX = globalPt.x() + button->boundingRect().width();
        int posY = globalPt.y() + 8 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_PREFERENCES_TAB_INFO);
        ti.x = posX;
        ti.y = posY;
        ti.title = text;
        ti.tailtype =  TOOLTIP_TAIL_LEFT;
        ti.tailPosPercent = 0.5;
        emit showTooltip(ti);
    }
}

void PreferencesTabControlItem::onButtonHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_PREFERENCES_TAB_INFO);
}

void PreferencesTabControlItem::fadeButtons(double newOpacity, int animationSpeed)
{
    generalButton_->setSelected(false);
    accountButton_->setSelected(false);
    connectionButton_->setSelected(false);
    shareButton_->setSelected(false);
    debugButton_->setSelected(false);

    generalButton_->animateOpacityChange(newOpacity, animationSpeed);
    accountButton_->animateOpacityChange(newOpacity, animationSpeed);
    connectionButton_->animateOpacityChange(newOpacity, animationSpeed);
    shareButton_->animateOpacityChange(newOpacity, animationSpeed);
    debugButton_->animateOpacityChange(newOpacity, animationSpeed);

}

void PreferencesTabControlItem::updateBottomAnchoredButtonPos()
{
    int quitButtonPosY = height_ - (SEPERATOR_Y + 16)*G_SCALE;
    int signOutButtonPosY = quitButtonPosY - (SEPERATOR_Y + 16)*G_SCALE;
    int helpButtonPosY = signOutButtonPosY - (SEPERATOR_Y + 16)*G_SCALE;

    helpButton_->setPos(buttonMarginX(), helpButtonPosY);
    signOutButton_->setPos(buttonMarginX(), signOutButtonPosY);
    quitButton_->setPos(buttonMarginX(), quitButtonPosY);
}

void PreferencesTabControlItem::updateTopAnchoredButtonsPos()
{
    generalButton_->setPos(buttonMarginX(), GENERAL_BUTTON_Y*G_SCALE);
    accountButton_->setPos(buttonMarginX(), ACCOUNT_BUTTON_Y*G_SCALE);
    connectionButton_->setPos(buttonMarginX(), CONNECTION_BUTTON_Y*G_SCALE);
    shareButton_->setPos(buttonMarginX(), SHARE_BUTTON_Y*G_SCALE);
    debugButton_->setPos(buttonMarginX(), DEBUG_BUTTON_Y*G_SCALE);
}

int PreferencesTabControlItem::buttonMarginX() const
{
    return (WIDTH - BUTTON_WIDTH) / 2 * G_SCALE;
}

void PreferencesTabControlItem::clearStickySelectionOnAllTabs()
{
    generalButton_->setStickySelection(false);
    accountButton_->setStickySelection(false);
    connectionButton_->setStickySelection(false);
    debugButton_->setStickySelection(false);
    shareButton_->setStickySelection(false);
}

}
