#include "welcomewindowitem.h"

#include <QPainter>
#include <QTextCursor>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/imageresourcesjpg.h"
#include "languagecontroller.h"
#include "utils/hardcodedsettings.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"


namespace LoginWindow {

const int LOGIN_MARGIN_WIDTH_LARGE = 24 ;

const QString EMERGENCY_ICON_ENABLED_PATH  = "login/EMERGENCY_ICON_ENABLED";
const QString EMERGENCY_ICON_DISABLED_PATH = "login/EMERGENCY_ICON_DISABLED";
const QString CONFIG_ICON_PATH             = "login/CONFIG_ICON";
const QString SETTINGS_ICON_PATH           = "login/SETTINGS_ICON";

WelcomeWindowItem::WelcomeWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    Q_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curBadgeScale_ = BADGE_SCALE_LARGE;
    curBadgePosX_ = centeredOffset(WINDOW_WIDTH, BADGE_WIDTH_BIG);
    curBadgePosY_ = centeredOffset(HEADER_HEIGHT-100, BADGE_HEIGHT_BIG);
    connect(&badgePosXAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBadgePosXChanged(QVariant)));
    connect(&badgePosYAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBadgePosYChanged(QVariant)));

    connect(&badgeScaleAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBadgeScaleChanged(QVariant)));

    curLoginTextOpacity_ = OPACITY_HIDDEN;
    connect(&loginTextOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onLoginTextOpacityChanged(QVariant)));

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, SIGNAL(clicked()), SLOT(onCloseClick()));

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, SIGNAL(clicked()), SLOT(onMinimizeClick()));
#else //if Q_OS_MAC

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, SIGNAL(clicked()), SLOT(onCloseClick()));
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, SIGNAL(clicked()), SLOT(onMinimizeClick()));
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

#endif

    curButtonLineXPos_ = LOGIN_MARGIN_WIDTH_LARGE;
    connect(&buttonLinePosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onButtonLinePosChanged(QVariant)));

    QString firewallOffText = QT_TRANSLATE_NOOP("LoginWindow::FirewallTurnOffButton", "Turn Off Firewall");
    firewallTurnOffButton_ = new FirewallTurnOffButton(std::move(firewallOffText), this);
    connect(firewallTurnOffButton_, SIGNAL(clicked()), SLOT(onFirewallTurnOffClick()));

    gotoLoginButton_ = new CommonGraphics::TextButton(tr("Login"), FontDescr(14, true), QColor(255, 255, 255), true, this );
    connect(gotoLoginButton_, SIGNAL(clicked()), SLOT(onGotoLoginButtonClick()));

    getStartedButton_ = new CommonGraphics::BubbleButtonBright(this, 130, 32, 15, 15);
    getStartedButton_->setText(tr("Get Started"));
    getStartedButton_->setFont(FontDescr(14, false));
    connect(getStartedButton_, SIGNAL(clicked()), SLOT(onGetStartedButtonClick()));

    curErrorOpacity_ = OPACITY_HIDDEN;
    connect(&errorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onErrorChanged(QVariant)));

    // Lower Region:
    settingsButton_ = new IconButton(24, 24, SETTINGS_ICON_PATH, "", this);
    connect(settingsButton_, SIGNAL(clicked()), SLOT(onSettingsButtonClick()));
    connect(settingsButton_, SIGNAL(hoverEnter()), SLOT(onSettingsHoverEnter()));
    connect(settingsButton_, SIGNAL(hoverLeave()), SLOT(onTooltipButtonHoverLeave()));

    configButton_ = new IconButton(24, 24, CONFIG_ICON_PATH, "", this);
    connect(configButton_, SIGNAL(clicked()), SLOT(onConfigButtonClick()));
    connect(configButton_, SIGNAL(hoverEnter()), SLOT(onConfigHoverEnter()));
    connect(configButton_, SIGNAL(hoverLeave()), SLOT(onTooltipButtonHoverLeave()));

    emergencyButton_ = new IconHoverEngageButton(EMERGENCY_ICON_DISABLED_PATH, EMERGENCY_ICON_ENABLED_PATH, this);
    connect(emergencyButton_, SIGNAL(clicked()), SLOT(onEmergencyButtonClick()));
    connect(emergencyButton_, SIGNAL(hoverEnter()), SLOT(onEmergencyHoverEnter()));
    connect(emergencyButton_, SIGNAL(hoverLeave()), SLOT(onTooltipButtonHoverLeave()));

    emergencyConnectOn_ = false;

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    curEmergencyTextOpacity_ = OPACITY_HIDDEN;
    connect(&emergencyTextAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onEmergencyTextTransition(QVariant)));

    connect(preferencesHelper, SIGNAL(isDockedModeChanged(bool)), this,
            SLOT(onDockedModeChanged(bool)));

    updatePositions();
    changeToAccountScreen();
}


void WelcomeWindowItem::updateEmergencyConnect()
{
    if (emergencyConnectOn_)
    {
        transitionToEmergencyON();
    }
    else
    {
        transitionToEmergencyOFF();
    }

    update();
}

void WelcomeWindowItem::setEmergencyConnectState(bool isEmergencyConnected)
{
    emergencyConnectOn_ = isEmergencyConnected;

    emergencyButton_->setActive(isEmergencyConnected);

    updateEmergencyConnect();
}

QRectF WelcomeWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void WelcomeWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    qreal initOpacity = painter->opacity();

    painter->save();

    QSharedPointer<IndependentPixmap> pixmap_background = ImageResourcesJpg::instance().getIndependentPixmap("welcome", WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
    pixmap_background->draw(0, 0, WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE, painter);

    painter->setOpacity(initOpacity);

    QSharedPointer<IndependentPixmap> pixmap_badge = ImageResourcesSvg::instance().getIndependentPixmap("BADGE_ICON");
    pixmap_badge->draw(curBadgePosX_ * G_SCALE, curBadgePosY_* G_SCALE, BADGE_WIDTH_BIG * G_SCALE, BADGE_HEIGHT_BIG * G_SCALE, painter);

    painter->setFont(*FontManager::instance().getFont(24, true));
    painter->setPen(Qt::white);
    painter->drawText(QRect(0, 138 * G_SCALE, WINDOW_WIDTH * G_SCALE, 40 * G_SCALE), Qt::AlignCenter, tr("Keep Your Secrets."));

    // dividers -- bottom buttons
    painter->setOpacity(OPACITY_UNHOVER_DIVIDER * initOpacity);
    const int bottom_button_y = LOGIN_HEIGHT * G_SCALE - settingsButton_->boundingRect().width();
    const int window_center_x_offset = WINDOW_WIDTH/2 * G_SCALE ;

    painter->fillRect(QRect(window_center_x_offset - 30*G_SCALE, bottom_button_y, 2*G_SCALE,  LOGIN_HEIGHT * G_SCALE), Qt::white);
    painter->fillRect(QRect(window_center_x_offset + 29*G_SCALE, bottom_button_y, 2*G_SCALE,  LOGIN_HEIGHT * G_SCALE), Qt::white);

    painter->restore();
}

void WelcomeWindowItem::resetState()
{
    /*changeToAccountScreen();
    updateEmergencyConnect();

    usernameEntry_->clearActiveState();
    passwordEntry_->clearActiveState();

    loginButton_->setError(false);
    loginButton_->setClickable(false);*/

}

void WelcomeWindowItem::setClickable(bool enabled)
{
    gotoLoginButton_->setClickable(enabled);
    getStartedButton_->setClickable(enabled);

    settingsButton_->setClickable(enabled);
    configButton_->setClickable(enabled);
    emergencyButton_->setClickable(enabled);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    minimizeButton_->setClickable(enabled);
    closeButton_->setClickable(enabled);
#endif
}

void WelcomeWindowItem::onBackClick()
{
    resetState();
}


void WelcomeWindowItem::onCloseClick()
{
    emit closeClick();
}

void WelcomeWindowItem::onMinimizeClick()
{
    emit minimizeClick();
}

void WelcomeWindowItem::onGotoLoginButtonClick()
{
    gotoLoginButton_->unhover();
    emit haveAccountYesClick();
}

void WelcomeWindowItem::onGetStartedButtonClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/signup?cpid=app_windows").arg(HardcodedSettings::instance().serverUrl())));
}

void WelcomeWindowItem::onSettingsButtonClick()
{
    emit preferencesClick();
}

void WelcomeWindowItem::onEmergencyButtonClick()
{
    emit emergencyConnectClick();
}

void WelcomeWindowItem::onConfigButtonClick()
{
    emit externalConfigModeClick();
}

void WelcomeWindowItem::onBadgePosXChanged(const QVariant &value)
{
    curBadgePosX_ = value.toInt();
    update();
}

void WelcomeWindowItem::onBadgePosYChanged(const QVariant &value)
{
    curBadgePosY_ = value.toInt();

    update();
}

void WelcomeWindowItem::onBadgeScaleChanged(const QVariant &value)
{
    curBadgeScale_ = value.toDouble();
    update();
}

void WelcomeWindowItem::onLoginTextOpacityChanged(const QVariant &value)
{
    curLoginTextOpacity_ = value.toDouble();
    update();
}

void WelcomeWindowItem::onButtonLinePosChanged(const QVariant &value)
{
    curButtonLineXPos_ = value.toInt();
    update();
}

void WelcomeWindowItem::onErrorChanged(const QVariant &value)
{
    curErrorOpacity_ = value.toDouble();
    update();
}

void WelcomeWindowItem::onEmergencyTextTransition(const QVariant &value)
{
    curEmergencyTextOpacity_ = value.toDouble();
    update();
}

void WelcomeWindowItem::showYesNo()
{
    gotoLoginButton_->setClickable(true);
    getStartedButton_->setClickable(true);
    gotoLoginButton_->show();
    getStartedButton_->show();
}

void WelcomeWindowItem::onHideYesNoTimerTick()
{
    gotoLoginButton_->setClickable(false);
    getStartedButton_->setClickable(false);
    gotoLoginButton_->hide();
    getStartedButton_->hide();
}

void WelcomeWindowItem::onEmergencyHoverEnter()
{
    onAbstractButtonHoverEnter(emergencyButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Emergency Connect"));
}

void WelcomeWindowItem::onConfigHoverEnter()
{
    onAbstractButtonHoverEnter(configButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "External Config"));
}

void WelcomeWindowItem::onSettingsHoverEnter()
{
    onAbstractButtonHoverEnter(settingsButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Preferences"));
}

void WelcomeWindowItem::onAbstractButtonHoverEnter(QGraphicsObject *button, QString text)
{
    if (button != nullptr)
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(button->scenePos()));

        int posX = globalPt.x() + 12 * G_SCALE;
        int posY = globalPt.y() -  2 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOGIN_ADDITIONAL_BUTTON_INFO);
        ti.x = posX;
        ti.y = posY;
        ti.title = text;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void WelcomeWindowItem::onLanguageChanged()
{
}

void WelcomeWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MAC)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void WelcomeWindowItem::updatePositions()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_->setPos((LOGIN_WIDTH - 16 - 16)*G_SCALE, 14*G_SCALE);
    minimizeButton_->setPos((LOGIN_WIDTH - 16 - 16 - 32)*G_SCALE, 14*G_SCALE);
    firewallTurnOffButton_->setPos(8*G_SCALE, 0);
#else
    minimizeButton_->setPos(28*G_SCALE, 8*G_SCALE);
    closeButton_->setPos(8*G_SCALE,8*G_SCALE);
    firewallTurnOffButton_->setPos((LOGIN_WIDTH - 8 - firewallTurnOffButton_->getWidth())*G_SCALE, 0);
#endif

    gotoLoginButton_->recalcBoundingRect();
    gotoLoginButton_->setPos((LOGIN_WIDTH*G_SCALE - gotoLoginButton_->boundingRect().width()) / 2, Y_COORD_NO_BUTTON*G_SCALE);
    getStartedButton_->setPos((LOGIN_WIDTH*G_SCALE - getStartedButton_->boundingRect().width()) / 2,  Y_COORD_YES_BUTTON*G_SCALE);



    int bottom_button_y = LOGIN_HEIGHT*G_SCALE - settingsButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE;
    int window_center_x_offset = WINDOW_WIDTH/2*G_SCALE - settingsButton_->boundingRect().width()/2;
    settingsButton_->setPos(window_center_x_offset - 58*G_SCALE, bottom_button_y);

    emergencyButton_->setPos(window_center_x_offset, bottom_button_y);

    configButton_->setPos(window_center_x_offset + 58*G_SCALE, bottom_button_y);
}

void WelcomeWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (hasFocus())
        {
            emit haveAccountYesClick();
        }
    }

    event->ignore();
}

bool WelcomeWindowItem::sceneEvent(QEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
    {

        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if ( keyEvent->key() == Qt::Key_Tab)
        {
            /*if (account_screen_)
            {
                if (yesButton_->hasFocus())
                {
                    yesButton_->animateButtonUnhighlight();
                    //noButton_->animateButtonHighlight();
                    return true;
                }
                else if (noButton_->hasFocus())
                {
                    //noButton_->animateButtonUnhighlight();
                    yesButton_->animateButtonHighlight();
                    return true;
                }
                else if (hasFocus())
                {
                    yesButton_->animateButtonHighlight();
                    return true;
                }
            }
            else
            {
                if (hasFocus())
                {
                    return true;
                }
            }*/
        }
    }

    QGraphicsItem::sceneEvent(event);
    return false;
}

void WelcomeWindowItem::changeToAccountScreen()
{
    startAnAnimation<int>(buttonLinePosAnimation_, curButtonLineXPos_, LOGIN_MARGIN_WIDTH_LARGE, ANIMATION_SPEED_SLOW );

    startAnAnimation<double>(badgePosXAnimation_, curBadgePosX_, centeredOffset(WINDOW_WIDTH, BADGE_WIDTH_BIG), ANIMATION_SPEED_SLOW);
    startAnAnimation<double>(badgePosYAnimation_, curBadgePosY_, centeredOffset(HEADER_HEIGHT+143, BADGE_HEIGHT_BIG), ANIMATION_SPEED_SLOW);
    startAnAnimation<double>(badgeScaleAnimation_, curBadgeScale_, BADGE_SCALE_LARGE, ANIMATION_SPEED_SLOW);

    startAnAnimation<double>(loginTextOpacityAnimation_, curLoginTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_SLOW);

    showYesNo();
    setFocus();
}

void WelcomeWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void WelcomeWindowItem::onTooltipButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOGIN_ADDITIONAL_BUTTON_INFO);
}

void WelcomeWindowItem::transitionToEmergencyON()
{
    double opacity = OPACITY_FULL;
     startAnAnimation<double>(emergencyTextAnimation_, curEmergencyTextOpacity_, opacity, ANIMATION_SPEED_FAST);
}

void WelcomeWindowItem::transitionToEmergencyOFF()
{
    startAnAnimation<double>(emergencyTextAnimation_, curEmergencyTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

int WelcomeWindowItem::centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

void WelcomeWindowItem::onFirewallTurnOffClick()
{
    firewallTurnOffButton_->animatedHide();
    emit firewallTurnOffClick();
}

void WelcomeWindowItem::setFirewallTurnOffButtonVisibility(bool visible)
{
    firewallTurnOffButton_->setActive(visible);
}

} // namespace LoginWindow

