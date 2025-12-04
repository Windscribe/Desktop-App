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
#include "graphicresources/imageresourcespng.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/ws_assert.h"
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
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &WelcomeWindowItem::onCloseClick);

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &WelcomeWindowItem::onMinimizeClick);
#else //if Q_OS_MACOS

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &WelcomeWindowItem::onCloseClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &WelcomeWindowItem::onMinimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

#endif

    firewallTurnOffButton_ = new FirewallTurnOffButton("", this);
    connect(firewallTurnOffButton_, &FirewallTurnOffButton::clicked, this, &WelcomeWindowItem::onFirewallTurnOffClick);

    getStartedButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kWelcome, 121, 38, 19);
    getStartedButton_->setFont(FontDescr(15, QFont::Medium));
    connect(getStartedButton_, &CommonGraphics::BubbleButton::clicked, this, &WelcomeWindowItem::onGetStartedButtonClick);
    getStartedButton_->setClickable(true);
    getStartedButton_->show();

    gotoLoginButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kWelcomeSecondary, 121, 38, 19);
    gotoLoginButton_->setFont(FontDescr(15, QFont::Medium));
    connect(gotoLoginButton_, &CommonGraphics::BubbleButton::clicked, this, &WelcomeWindowItem::onGotoLoginButtonClick);
    gotoLoginButton_->setClickable(true);
    gotoLoginButton_->show();

    // Lower Region:
    emergencyButton_ = new IconHoverEngageButton(EMERGENCY_ICON_DISABLED_PATH, EMERGENCY_ICON_ENABLED_PATH, this);
    connect(emergencyButton_, &IconHoverEngageButton::clicked, this, &WelcomeWindowItem::onEmergencyButtonClick);
    connect(emergencyButton_, &IconHoverEngageButton::hoverEnter, this, &WelcomeWindowItem::onEmergencyHoverEnter);
    connect(emergencyButton_, &IconHoverEngageButton::hoverLeave, this, &WelcomeWindowItem::onTooltipButtonHoverLeave);

    settingsButton_ = new IconButton(24, 24, SETTINGS_ICON_PATH, "", this);
    connect(settingsButton_, &IconButton::clicked, this, &WelcomeWindowItem::onSettingsButtonClick);
    connect(settingsButton_, &IconButton::hoverEnter, this, &WelcomeWindowItem::onSettingsHoverEnter);
    connect(settingsButton_, &IconButton::hoverLeave, this, &WelcomeWindowItem::onTooltipButtonHoverLeave);

    configButton_ = new IconButton(24, 24, CONFIG_ICON_PATH, "", this);
    connect(configButton_, &IconButton::clicked, this, &WelcomeWindowItem::onConfigButtonClick);
    connect(configButton_, &IconButton::hoverEnter, this, &WelcomeWindowItem::onConfigHoverEnter);
    connect(configButton_, &IconButton::hoverLeave, this, &WelcomeWindowItem::onTooltipButtonHoverLeave);

    emergencyConnectOn_ = false;

    curEmergencyTextOpacity_ = OPACITY_HIDDEN;
    connect(&emergencyTextAnimation_, &QVariantAnimation::valueChanged, this, &WelcomeWindowItem::onEmergencyTextTransition);

    connect(preferencesHelper, &PreferencesHelper::isDockedModeChanged, this, &WelcomeWindowItem::onDockedModeChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &WelcomeWindowItem::onLanguageChanged);
    onLanguageChanged();

    updatePositions();
    setFocus();
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
    painter->setOpacity(OPACITY_FULL);

    QPainterPath path;
    path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
    painter->setPen(Qt::NoPen);
    painter->fillPath(path, QColor(9, 15, 25));
    painter->setPen(Qt::SolidLine);

    painter->setCompositionMode(QPainter::CompositionMode_SourceIn);
    QSharedPointer<IndependentPixmap> pixmap_background = ImageResourcesPng::instance().getIndependentPixmap("welcome");
    pixmap_background->draw(boundingRect().toRect(), painter);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    QSharedPointer<IndependentPixmap> pixmap_logo = ImageResourcesSvg::instance().getIndependentPixmap("LOGO");
    pixmap_logo->draw(centeredOffset(WINDOW_WIDTH, LOGO_WIDTH)*G_SCALE, LOGO_POS_Y*G_SCALE, LOGO_WIDTH*G_SCALE, LOGO_HEIGHT*G_SCALE, painter);

    // Border
    QSharedPointer<IndependentPixmap> pixmapBorder = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_TOP_INNER_VAN_GOGH");
    pixmapBorder->draw(0, 0, 350*G_SCALE, 32*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> pixmapBorderExtension = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_TOP_INNER_EXTENSION");
    pixmapBorderExtension->draw(0, 32*G_SCALE, 350*G_SCALE, 286*G_SCALE, painter);

    QSharedPointer<IndependentPixmap> pixmapBorderFooter = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_BOTTOM_INNER");
    pixmapBorderFooter->draw(0, 318*G_SCALE, 350*G_SCALE, 32*G_SCALE, painter);
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
    QDesktopServices::openUrl(QUrl( QString("https://%1/signup?cpid=app_windows").arg(HardcodedSettings::instance().windscribeServerUrl())));
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

void WelcomeWindowItem::onEmergencyTextTransition(const QVariant &value)
{
    curEmergencyTextOpacity_ = value.toDouble();
    update();
}

void WelcomeWindowItem::onEmergencyHoverEnter()
{
    onAbstractButtonHoverEnter(emergencyButton_, tr("Emergency Connect"));
}

void WelcomeWindowItem::onConfigHoverEnter()
{
    onAbstractButtonHoverEnter(configButton_, tr("External Config"));
}

void WelcomeWindowItem::onSettingsHoverEnter()
{
    onAbstractButtonHoverEnter(settingsButton_, tr("Preferences"));
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
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void WelcomeWindowItem::onLanguageChanged()
{
    firewallTurnOffButton_->setText(tr("Turn Off Firewall"));
    gotoLoginButton_->setText(tr("Login"));
    getStartedButton_->setText(tr("Get Started"));
    updatePositions();
}

void WelcomeWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MACOS)
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
    minimizeButton_->setPos(38*G_SCALE, 16*G_SCALE);
    closeButton_->setPos(16*G_SCALE, 16*G_SCALE);
    firewallTurnOffButton_->setPos((LOGIN_WIDTH - 16 - firewallTurnOffButton_->getWidth())*G_SCALE, 0);
#endif

    getStartedButton_->setPos((LOGIN_WIDTH*G_SCALE - getStartedButton_->boundingRect().width()) / 2, GET_STARTED_BUTTON_POS_Y*G_SCALE);
    gotoLoginButton_->setPos((LOGIN_WIDTH*G_SCALE - gotoLoginButton_->boundingRect().width()) / 2, LOGIN_BUTTON_POS_Y*G_SCALE);

    int bottom_button_y = LOGIN_HEIGHT*G_SCALE - settingsButton_->boundingRect().width() - 24*G_SCALE;
    int window_center_x_offset = WINDOW_WIDTH/2*G_SCALE - settingsButton_->boundingRect().width()/2;
    emergencyButton_->setPos(window_center_x_offset - 48*G_SCALE, bottom_button_y);
    settingsButton_->setPos(window_center_x_offset, bottom_button_y);
    configButton_->setPos(window_center_x_offset + 48*G_SCALE, bottom_button_y);
}

void WelcomeWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (hasFocus()) {
            emit haveAccountYesClick();
        }
    }

    event->ignore();
}

void WelcomeWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    updatePositions();
    update();
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

