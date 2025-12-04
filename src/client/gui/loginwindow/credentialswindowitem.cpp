#include "credentialswindowitem.h"

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
#include "languagecontroller.h"
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"

namespace LoginWindow {

const QString EMERGENCY_ICON_ENABLED_PATH  = "login/EMERGENCY_ICON_ENABLED";
const QString EMERGENCY_ICON_DISABLED_PATH = "login/EMERGENCY_ICON_DISABLED";
const QString CONFIG_ICON_PATH             = "login/CONFIG_ICON";
const QString SETTINGS_ICON_PATH           = "login/SETTINGS_ICON";

CredentialsWindowItem::CredentialsWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), curError_(LoginWindow::ERR_MSG_EMPTY), curErrorMsg_("")
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    // Header Region:
    backButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backButton_, &IconButton::clicked, this, &CredentialsWindowItem::onBackClick);

    curLoginTextOpacity_ = OPACITY_HIDDEN;
    connect(&loginTextOpacityAnimation_, &QVariantAnimation::valueChanged, this, &CredentialsWindowItem::onLoginTextOpacityChanged);

 #if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &CredentialsWindowItem::onCloseClick);

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &CredentialsWindowItem::onMinimizeClick);
#else //if Q_OS_MACOS

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &CredentialsWindowItem::onCloseClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &CredentialsWindowItem::onMinimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

#endif

    QString firewallOffText = tr("Turn Off Firewall");
    firewallTurnOffButton_ = new FirewallTurnOffButton(std::move(firewallOffText), this);
    connect(firewallTurnOffButton_, &FirewallTurnOffButton::clicked, this, &CredentialsWindowItem::onFirewallTurnOffClick);

    // Center Region:
    usernameEntry_ = new UsernamePasswordEntry("", false, this);
    passwordEntry_ = new UsernamePasswordEntry("", true, this);

    curForgotAnd2FAPosY_ = FORGOT_AND_2FA_POS_Y_DEFAULT;
    connect(&forgotAnd2FAPosYAnimation_, &QVariantAnimation::valueChanged, this, &CredentialsWindowItem::onForgotAnd2FAPosYChanged);

    curErrorOpacity_ = OPACITY_HIDDEN;
    connect(&errorAnimation_, &QVariantAnimation::valueChanged, this, &CredentialsWindowItem::onErrorChanged);

    twoFactorAuthButton_ = new CommonGraphics::TextButton("", FontDescr(12, QFont::Normal), Qt::white, true, this);
    connect(twoFactorAuthButton_, &CommonGraphics::TextButton::clicked, this, &CredentialsWindowItem::onTwoFactorAuthClick);
    twoFactorAuthButton_->quickHide();

    forgotPassButton_ = new CommonGraphics::TextButton("", FontDescr(12, QFont::Normal), Qt::white, true, this);
    connect(forgotPassButton_, &CommonGraphics::TextButton::clicked, this, &CredentialsWindowItem::onForgotPassClick);
    forgotPassButton_->quickHide();

    loginButton_ = new LoginButton(this);
    connect(loginButton_, &LoginButton::clicked, this, &CredentialsWindowItem::onLoginClick);

    // Lower Region:
    settingsButton_ = new IconButton(24, 24, SETTINGS_ICON_PATH, "", this);
    connect(settingsButton_, &IconButton::clicked, this, &CredentialsWindowItem::onSettingsButtonClick);
    connect(settingsButton_, &IconButton::hoverEnter, this, &CredentialsWindowItem::onSettingsHoverEnter);
    connect(settingsButton_, &IconButton::hoverLeave, this, &CredentialsWindowItem::onTooltipButtonHoverLeave);

    configButton_ = new IconButton(24, 24, CONFIG_ICON_PATH, "", this);
    connect(configButton_, &IconButton::clicked, this, &CredentialsWindowItem::onConfigButtonClick);
    connect(configButton_, &IconButton::hoverEnter, this, &CredentialsWindowItem::onConfigHoverEnter);
    connect(configButton_, &IconButton::hoverLeave, this, &CredentialsWindowItem::onTooltipButtonHoverLeave);

    emergencyButton_ = new IconHoverEngageButton(EMERGENCY_ICON_DISABLED_PATH, EMERGENCY_ICON_ENABLED_PATH, this);
    connect(emergencyButton_, &IconHoverEngageButton::clicked, this, &CredentialsWindowItem::onEmergencyButtonClick);
    connect(emergencyButton_, &IconHoverEngageButton::hoverEnter, this, &CredentialsWindowItem::onEmergencyHoverEnter);
    connect(emergencyButton_, &IconHoverEngageButton::hoverLeave, this, &CredentialsWindowItem::onTooltipButtonHoverLeave);

    emergencyConnectOn_ = false;

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &CredentialsWindowItem::onLanguageChanged);
    onLanguageChanged();

    curEmergencyTextOpacity_ = OPACITY_HIDDEN;
    connect(&emergencyTextAnimation_, &QVariantAnimation::valueChanged, this, &CredentialsWindowItem::onEmergencyTextTransition);

    connect(usernameEntry_, &UsernamePasswordEntry::textChanged, this, &CredentialsWindowItem::onUsernamePasswordTextChanged);
    connect(passwordEntry_, &UsernamePasswordEntry::textChanged, this, &CredentialsWindowItem::onUsernamePasswordTextChanged);

    connect(preferencesHelper, &PreferencesHelper::isDockedModeChanged, this, &CredentialsWindowItem::onDockedModeChanged);

    updatePositions();
    transitionToUsernameScreen();
}

void CredentialsWindowItem::setErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    bool error = false;

    switch(errorMessageType)
    {
        case LoginWindow::ERR_MSG_EMPTY:
            curErrorText_.clear();
            break;
        case LoginWindow::ERR_MSG_NO_INTERNET_CONNECTIVITY:
            curErrorText_ = tr("No Internet Connectivity");
            break;
        case LoginWindow::ERR_MSG_NO_API_CONNECTIVITY:
            curErrorText_ = tr("No API Connectivity");
            break;
        case LoginWindow::ERR_MSG_PROXY_REQUIRES_AUTH:
            curErrorText_ = tr("Proxy requires authentication");
            break;
        case LoginWindow::ERR_MSG_INVALID_API_RESPONSE:
            curErrorText_ = tr("Invalid API response, check your network");
            break;
        case LoginWindow::ERR_MSG_INVALID_API_ENDPOINT:
            curErrorText_ = tr("Invalid API Endpoint");
            break;
        case LoginWindow::ERR_MSG_INCORRECT_LOGIN1:
            error = true;
            curErrorText_ = tr("...hmm are you sure this is correct?");
            break;
        case LoginWindow::ERR_MSG_INCORRECT_LOGIN2:
            error = true;
            curErrorText_ = tr("...Sorry, seems like it's wrong again");
            break;
        case LoginWindow::ERR_MSG_INCORRECT_LOGIN3:
            error = true;
            curErrorText_ = tr("...hmm, try resetting your password!");
            break;
        case LoginWindow::ERR_MSG_RATE_LIMITED:
            error = true;
            curErrorText_ = tr("Rate limited. Please wait before trying to login again.");
            break;
        case LoginWindow::ERR_MSG_SESSION_EXPIRED:
            curErrorText_ = tr("Session is expired. Please login again");
            break;
        case LoginWindow::ERR_MSG_USERNAME_IS_EMAIL:
            curErrorText_ = tr("Your username should not be an email address. Please try again.");
            break;
        case LoginWindow::ERR_MSG_ACCOUNT_DISABLED:
            curErrorText_ = errorMessage;
            break;
        case LoginWindow::ERR_MSG_INVALID_SECURITY_TOKEN:
            curErrorText_ = errorMessage;
            break;
        default:
            WS_ASSERT(false);
            break;
    }

    curError_ = errorMessageType;
    curErrorMsg_ = errorMessage;

    usernameEntry_->setError(error);
    passwordEntry_->setError(error);

    loginButton_->setError(error);

    int destinationForgotAnd2FAPosY = FORGOT_AND_2FA_POS_Y_DEFAULT;
    double errorOpacity = OPACITY_HIDDEN;
    if (error || !curErrorText_.isEmpty())
    {
        errorOpacity = OPACITY_FULL;
        destinationForgotAnd2FAPosY = FORGOT_AND_2FA_POS_Y_ERROR;
    }

    startAnAnimation<int>(forgotAnd2FAPosYAnimation_, curForgotAnd2FAPosY_,
        destinationForgotAnd2FAPosY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(errorAnimation_, curErrorOpacity_, errorOpacity, ANIMATION_SPEED_FAST);

    update();
}

void CredentialsWindowItem::updateEmergencyConnect()
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

void CredentialsWindowItem::setEmergencyConnectState(bool isEmergencyConnected)
{
    emergencyConnectOn_ = isEmergencyConnected;

    emergencyButton_->setActive(isEmergencyConnected);

    updateEmergencyConnect();
}

QRectF CredentialsWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void CredentialsWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    qreal initOpacity = painter->opacity();


    QPainterPath path;
    path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
    painter->setPen(Qt::NoPen);
    painter->fillPath(path, QColor(9, 15, 25));
    painter->setPen(Qt::SolidLine);

    QRectF rcBottomRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, (LOGIN_HEIGHT - HEADER_HEIGHT)*G_SCALE);

    QColor darkblue = FontManager::instance().getDarkBlueColor();
    painter->setBrush(darkblue);
    painter->setPen(darkblue);
    painter->drawRoundedRect(rcBottomRect, 9*G_SCALE, 9*G_SCALE);

    // We don't actually want the upper corners of the bottom rect to be rounded.  Fill them in
    painter->fillRect(QRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, 9*G_SCALE), darkblue);

    // Login Text
    painter->save();
    painter->setOpacity(curLoginTextOpacity_ * initOpacity);
    painter->setFont(FontManager::instance().getFont(24,  QFont::Normal));
    painter->setPen(QColor(255,255,255)); //  white

    QString loginText = tr("Login");
    QFontMetrics fm = painter->fontMetrics();
    const int loginTextWidth = fm.horizontalAdvance(loginText);
    painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, loginTextWidth),
                        (HEADER_HEIGHT/2 + LOGIN_TEXT_HEIGHT/2)*G_SCALE,
                        loginText);
    painter->restore();

    // Error Text
    painter->setFont(FontManager::instance().getFont(12,  QFont::Normal));
    painter->setPen(FontManager::instance().getErrorRedColor());
    painter->setOpacity(curErrorOpacity_ * initOpacity);
    QRectF rect(WINDOW_MARGIN*G_SCALE, 232*G_SCALE, WINDOW_WIDTH*G_SCALE - loginButton_->boundingRect().width() - 48*G_SCALE, 32*G_SCALE);
    painter->drawText(rect, Qt::AlignVCenter | Qt::TextWordWrap, curErrorText_);

    // Emergency Connect Text
    painter->save();
    painter->setPen(FontManager::instance().getBrightBlueColor());
    painter->setOpacity(curEmergencyTextOpacity_ * initOpacity);
    painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, EMERGENCY_CONNECT_TEXT_WIDTH*G_SCALE), EMERGENCY_CONNECT_TEXT_POS_Y*G_SCALE, tr("Emergency Connect is ON"));
    painter->restore();

    // divider -- between 2FA and forgot password.
    if (twoFactorAuthButton_->getOpacity() != OPACITY_HIDDEN) {
        fm = painter->fontMetrics();
        const int forgot_and_2fa_divider_x = (WINDOW_MARGIN*G_SCALE
            + twoFactorAuthButton_->boundingRect().width() + forgotPassButton_->x()) / 2;
        const int forgot_and_2fa_divider_y = ( 1 + curForgotAnd2FAPosY_ ) * G_SCALE
            + (twoFactorAuthButton_->boundingRect().height() - fm.ascent()) / 2;
        painter->fillRect(QRect(forgot_and_2fa_divider_x, forgot_and_2fa_divider_y,
                                1 * G_SCALE, fm.ascent()), Qt::white);
    }
}

void CredentialsWindowItem::resetState()
{
    /*updateEmergencyConnect();*/

    usernameEntry_->clearActiveState();
    passwordEntry_->clearActiveState();

    loginButton_->setError(false);
    loginButton_->setClickable(false);

    setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString()); // reset error state

    curForgotAnd2FAPosY_ = FORGOT_AND_2FA_POS_Y_DEFAULT;
    twoFactorAuthButton_->setPos(WINDOW_MARGIN * G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
            + (WINDOW_MARGIN + FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
}

void CredentialsWindowItem::setClickable(bool enabled)
{
    if (enabled)
    {
        usernameEntry_->setClickable(enabled);
        passwordEntry_->setClickable(enabled);
        twoFactorAuthButton_->setClickable(enabled);
        forgotPassButton_->setClickable(enabled);

        if (isUsernameAndPasswordValid())
        {
            loginButton_->setClickable(enabled);
        }
    }
    else
    {
        usernameEntry_->setClickable(enabled);
        passwordEntry_->setClickable(enabled);
        twoFactorAuthButton_->setClickable(enabled);
        forgotPassButton_->setClickable(enabled);
        loginButton_->setClickable(enabled);

    }

    backButton_->setClickable(enabled);
    settingsButton_->setClickable(enabled);
    configButton_->setClickable(enabled);
    emergencyButton_->setClickable(enabled);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    minimizeButton_->setClickable(enabled);
    closeButton_->setClickable(enabled);
#endif
}

void CredentialsWindowItem::setCurrent2FACode(QString code)
{
    current2FACode_ = code;
}

void CredentialsWindowItem::onBackClick()
{
    emit backClick();
}

void CredentialsWindowItem::attemptLogin()
{
    QString username = usernameEntry_->getText();
    QString password = passwordEntry_->getText();

    usernameEntry_->setError(false);
    passwordEntry_->setError(false);

    bool userOrPassError = false;
    if (username.length() < 3)
    {
        userOrPassError = true;
    }
    else if (password.length() < 4) // activate with text
    {
        userOrPassError = true;
    }

    if (userOrPassError)
    {
        setErrorMessage(LoginWindow::ERR_MSG_INCORRECT_LOGIN1, QString());
    }
    else
    {
        emit loginClick(username, password, current2FACode_);
    }

}

void CredentialsWindowItem::onLoginClick()
{
    attemptLogin();
}

void CredentialsWindowItem::onCloseClick()
{
    emit closeClick();
}

void CredentialsWindowItem::onMinimizeClick()
{
    emit minimizeClick();
}

void CredentialsWindowItem::onGotoLoginButtonClick()
{
    transitionToUsernameScreen();
    updateEmergencyConnect();

    emit haveAccountYesClick();
}

void CredentialsWindowItem::onGetStartedButtonClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/signup?cpid=app_windows").arg(HardcodedSettings::instance().windscribeServerUrl())));
}

void CredentialsWindowItem::onTwoFactorAuthClick()
{
    QString username = usernameEntry_->getText();
    QString password = passwordEntry_->getText();
    twoFactorAuthButton_->unhover();
    emit twoFactorAuthClick(username, password);
}

void CredentialsWindowItem::onForgotPassClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/forgotpassword").arg(HardcodedSettings::instance().windscribeServerUrl())));
}

void CredentialsWindowItem::onSettingsButtonClick()
{
    emit preferencesClick();
}

void CredentialsWindowItem::onEmergencyButtonClick()
{
    emit emergencyConnectClick();
}

void CredentialsWindowItem::onConfigButtonClick()
{
    emit externalConfigModeClick();
}

void CredentialsWindowItem::onLoginTextOpacityChanged(const QVariant &value)
{
    curLoginTextOpacity_ = value.toDouble();
    update();
}

void CredentialsWindowItem::onForgotAnd2FAPosYChanged(const QVariant &value)
{
    curForgotAnd2FAPosY_ = value.toInt();

    twoFactorAuthButton_->setPos(WINDOW_MARGIN*G_SCALE, curForgotAnd2FAPosY_*G_SCALE);
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
        + (WINDOW_MARGIN+FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_*G_SCALE);

    update();
}

void CredentialsWindowItem::onErrorChanged(const QVariant &value)
{
    curErrorOpacity_ = value.toDouble();
    update();
}

void CredentialsWindowItem::onEmergencyTextTransition(const QVariant &value)
{
    curEmergencyTextOpacity_ = value.toDouble();
    update();
}

void CredentialsWindowItem::hideUsernamePassword()
{
    usernameEntry_->setClickable(false);
    passwordEntry_->setClickable(false);
    usernameEntry_->hide();
    passwordEntry_->hide();
}

void CredentialsWindowItem::showUsernamePassword()
{
    usernameEntry_->setClickable(true);
    passwordEntry_->setClickable(true);
    usernameEntry_->show();
    passwordEntry_->show();
}

void CredentialsWindowItem::onUsernamePasswordTextChanged(const QString & /*text*/)
{
    loginButton_->setClickable(isUsernameAndPasswordValid());
}

void CredentialsWindowItem::onEmergencyHoverEnter()
{
    onAbstractButtonHoverEnter(emergencyButton_, tr("Emergency Connect"));
}

void CredentialsWindowItem::onConfigHoverEnter()
{
    onAbstractButtonHoverEnter(configButton_, tr("External Config"));
}

void CredentialsWindowItem::onSettingsHoverEnter()
{
    onAbstractButtonHoverEnter(settingsButton_, tr("Preferences"));
}

void CredentialsWindowItem::onAbstractButtonHoverEnter(QGraphicsObject *button, QString text)
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

void CredentialsWindowItem::onLanguageChanged()
{
    twoFactorAuthButton_->setText(tr("2FA Code"));
    twoFactorAuthButton_->recalcBoundingRect();
    forgotPassButton_->setText(tr("Forgot password?"));
    forgotPassButton_->recalcBoundingRect();
    usernameEntry_->setText(tr("Username"));
    passwordEntry_->setText(tr("Password"));
    setErrorMessage(curError_, curErrorMsg_);
}

void CredentialsWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MACOS)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void CredentialsWindowItem::updatePositions()
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

    backButton_->setPos(16*G_SCALE, 28*G_SCALE);

    int bottom_button_y = LOGIN_HEIGHT*G_SCALE - settingsButton_->boundingRect().width() - 24*G_SCALE;
    int window_center_x_offset = WINDOW_WIDTH/2*G_SCALE - settingsButton_->boundingRect().width()/2;

    emergencyButton_->setPos(window_center_x_offset - 48*G_SCALE, bottom_button_y);
    settingsButton_->setPos(window_center_x_offset, bottom_button_y);
    configButton_->setPos(window_center_x_offset + 48*G_SCALE, bottom_button_y);

    usernameEntry_->setPos(0, USERNAME_POS_Y_VISIBLE*G_SCALE);
    passwordEntry_->setPos(0, PASSWORD_POS_Y_VISIBLE*G_SCALE);

    loginButton_->setPos((LOGIN_WIDTH - 32 - WINDOW_MARGIN)*G_SCALE, LOGIN_BUTTON_POS_Y*G_SCALE);

    twoFactorAuthButton_->setPos(WINDOW_MARGIN * G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    twoFactorAuthButton_->recalcBoundingRect();
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
        + (WINDOW_MARGIN + FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    forgotPassButton_->recalcBoundingRect();
}

void CredentialsWindowItem::keyPressEvent(QKeyEvent *event)
{
    // qDebug() << "Credentials::keyPressEvent";
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        attemptLogin();
    }
    else if (event->key() == Qt::Key_Escape)
    {
        emit backClick();
    }

    event->ignore();
}

bool CredentialsWindowItem::sceneEvent(QEvent *event)
{
    // qDebug() << "Credentials:: scene event";
    if (event->type() == QEvent::KeyRelease)
    {

        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if ( keyEvent->key() == Qt::Key_Tab)
        {
            if (hasFocus())
            {
                //qDebug() << "Credentials::setting focus";
                usernameEntry_->setFocus();
                return true;
            }
        }
    }

    QGraphicsItem::sceneEvent(event);
    return false;
}

void CredentialsWindowItem::transitionToUsernameScreen()
{
    backButton_->setOpacity(OPACITY_FULL);
    usernameEntry_->setOpacityByFactor(OPACITY_FULL);
    passwordEntry_->setOpacityByFactor(OPACITY_FULL);
    loginButton_->setOpacity(OPACITY_FULL);

    startAnAnimation<double>(loginTextOpacityAnimation_, curLoginTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_SLOW);

    twoFactorAuthButton_->animateShow(ANIMATION_SPEED_SLOW);
    forgotPassButton_->animateShow(ANIMATION_SPEED_SLOW);

    showUsernamePassword();

    QTimer::singleShot(ANIMATION_SPEED_SLOW, [this]() { usernameEntry_->setFocus(); });
}

void CredentialsWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
    usernameEntry_->updateScaling();
    passwordEntry_->updateScaling();
}

void CredentialsWindowItem::setUsernameFocus()
{
    // qDebug() << "Credentials::setUsernameFocus()";
    usernameEntry_->setFocus();
}

void CredentialsWindowItem::onTooltipButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOGIN_ADDITIONAL_BUTTON_INFO);
}

void CredentialsWindowItem::transitionToEmergencyON()
{
    double opacity = OPACITY_HIDDEN;
    startAnAnimation<double>(emergencyTextAnimation_, curEmergencyTextOpacity_, opacity, ANIMATION_SPEED_FAST);
}

void CredentialsWindowItem::transitionToEmergencyOFF()
{
    startAnAnimation<double>(emergencyTextAnimation_, curEmergencyTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

int CredentialsWindowItem::centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

bool CredentialsWindowItem::isUsernameAndPasswordValid() const
{
    return !usernameEntry_->getText().isEmpty() && !passwordEntry_->getText().isEmpty();
}

void CredentialsWindowItem::onFirewallTurnOffClick()
{
    firewallTurnOffButton_->animatedHide();
    emit firewallTurnOffClick();
}

void CredentialsWindowItem::setFirewallTurnOffButtonVisibility(bool visible)
{
    firewallTurnOffButton_->setActive(visible);
}

} // namespace LoginWindow

