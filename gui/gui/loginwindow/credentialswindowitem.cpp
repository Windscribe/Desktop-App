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

CredentialsWindowItem::CredentialsWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    Q_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    // Header Region:
    backButton_ = new IconButton(16,16, "login/BACK_ARROW", this);
    connect(backButton_, SIGNAL(clicked()), SLOT(onBackClick()));

    curLoginTextOpacity_ = OPACITY_HIDDEN;
    connect(&loginTextOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onLoginTextOpacityChanged(QVariant)));

 #ifdef Q_OS_WIN
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", this);
    connect(closeButton_, SIGNAL(clicked()), SLOT(onCloseClick()));

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", this);
    connect(minimizeButton_, SIGNAL(clicked()), SLOT(onMinimizeClick()));
#else //if Q_OS_MAC

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", this);
    connect(closeButton_, SIGNAL(clicked()), SLOT(onCloseClick()));
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", this);
    connect(minimizeButton_, SIGNAL(clicked()), SLOT(onMinimizeClick()));
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

#endif

    QString firewallOffText = QT_TRANSLATE_NOOP("LoginWindow::FirewallTurnOffButton", "Turn Off Firewall");
    firewallTurnOffButton_ = new FirewallTurnOffButton(std::move(firewallOffText), this);
    connect(firewallTurnOffButton_, SIGNAL(clicked()), SLOT(onFirewallTurnOffClick()));

    // Center Region:
    QString usernameText = QT_TRANSLATE_NOOP("LoginWindow::UsernamePasswordEntry", "Username");
    QString passwordText = QT_TRANSLATE_NOOP("LoginWindow::UsernamePasswordEntry", "Password");

    usernameEntry_ = new UsernamePasswordEntry(usernameText, false, this);
    passwordEntry_ = new UsernamePasswordEntry(passwordText, true, this);

    curForgotAnd2FAPosY_ = FORGOT_AND_2FA_POS_Y_DEFAULT;
    connect(&forgotAnd2FAPosYAnimation_, SIGNAL(valueChanged(QVariant)),
        SLOT(onForgotAnd2FAPosYChanged(QVariant)));

    curErrorOpacity_ = OPACITY_HIDDEN;
    connect(&errorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onErrorChanged(QVariant)));

    QString twoFactorAuthText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "2FA Code");
    twoFactorAuthButton_ = new CommonGraphics::TextButton(twoFactorAuthText, FontDescr(12, false),
                                                          Qt::white, true, this);
    connect(twoFactorAuthButton_, SIGNAL(clicked()), SLOT(onTwoFactorAuthClick()));
    twoFactorAuthButton_->quickHide();

    QString forgotPassText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Forgot password?");
    forgotPassButton_ = new CommonGraphics::TextButton(forgotPassText, FontDescr(12, false),
                                                       Qt::white, true, this);
    connect(forgotPassButton_, SIGNAL(clicked()), SLOT(onForgotPassClick()));
    forgotPassButton_->quickHide();

    loginButton_ = new LoginButton(this);
    connect(loginButton_, SIGNAL(clicked()), SLOT(onLoginClick()));

    // Lower Region:
    settingsButton_ = new IconButton(24, 24, SETTINGS_ICON_PATH, this);
    connect(settingsButton_, SIGNAL(clicked()), SLOT(onSettingsButtonClick()));
    connect(settingsButton_, SIGNAL(hoverEnter()), SLOT(onSettingsHoverEnter()));
    connect(settingsButton_, SIGNAL(hoverLeave()), SLOT(onTooltipButtonHoverLeave()));

    configButton_ = new IconButton(24, 24, CONFIG_ICON_PATH, this);
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

    connect(usernameEntry_, SIGNAL(textChanged(const QString &)), this, SLOT(onUsernamePasswordTextChanged(const QString &)));
    connect(passwordEntry_, SIGNAL(textChanged(const QString &)), this, SLOT(onUsernamePasswordTextChanged(const QString &)));

    connect(preferencesHelper, SIGNAL(isDockedModeChanged(bool)), this,
            SLOT(onDockedModeChanged(bool)));

    updatePositions();
    transitionToUsernameScreen();
}

void CredentialsWindowItem::setErrorMessage(ILoginWindow::ERROR_MESSAGE_TYPE errorMessage)
{
    bool error = false;

    switch(errorMessage)
    {
        case ILoginWindow::ERR_MSG_EMPTY:
            curErrorText_.clear();
            break;
        case ILoginWindow::ERR_MSG_NO_INTERNET_CONNECTIVITY:
            curErrorText_ = tr("No Internet Connectivity");
            break;
        case ILoginWindow::ERR_MSG_NO_API_CONNECTIVITY:
            curErrorText_ = tr("No API Connectivity");
            break;
        case ILoginWindow::ERR_MSG_PROXY_REQUIRES_AUTH:
            curErrorText_ = tr("Proxy requires authentication");
            break;
        case ILoginWindow::ERR_MSG_INVALID_API_RESPONCE:
            curErrorText_ = tr("Invalid API response, check your network");
            break;
        case ILoginWindow::ERR_MSG_INVALID_API_ENDPOINT:
            curErrorText_ = tr("Invalid API Endpoint");
            break;
        case ILoginWindow::ERR_MSG_INCORRECT_LOGIN1:
            error = true;
            curErrorText_ = tr("...hmm are you sure this is correct?");
            break;
        case ILoginWindow::ERR_MSG_INCORRECT_LOGIN2:
            error = true;
            curErrorText_ = tr("...Sorry, seems like it's wrong again");
            break;
        case ILoginWindow::ERR_MSG_INCORRECT_LOGIN3:
            error = true;
            curErrorText_ = tr("...hmm, try reseting your password!");
            break;
        case ILoginWindow::ERR_MSG_SESSION_EXPIRED:
            curErrorText_ = tr("Session is expired. Please login again");
            break;
        default:
            Q_ASSERT(false);
            break;
    }

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

    #ifdef Q_OS_WIN
        const int pushDownSquare = 0;
    #else
        const int pushDownSquare = 5;
    #endif

        QRectF rcTopRect(0, 0, WINDOW_WIDTH*G_SCALE, HEADER_HEIGHT*G_SCALE);
        QRectF rcBottomRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, (LOGIN_HEIGHT - HEADER_HEIGHT)*G_SCALE);
        QColor black = FontManager::instance().getMidnightColor();
        QColor darkblue = FontManager::instance().getDarkBlueColor();

    #ifndef Q_OS_WIN // round background
        painter->setPen(black);
        painter->setBrush(black);
        painter->drawRoundedRect(rcTopRect, 5*G_SCALE, 5*G_SCALE);

        painter->setBrush(darkblue);
        painter->setPen(darkblue);
        painter->drawRoundedRect(rcBottomRect.adjusted(0, 5*G_SCALE, 0, 0), 5*G_SCALE, 5*G_SCALE);
    #endif

        // Square background
        painter->fillRect(rcTopRect.adjusted(0, pushDownSquare, 0, 0), black);
        painter->fillRect(rcBottomRect.adjusted(0,0,0, -pushDownSquare*2), darkblue);

        // Login Text
        painter->save();
        painter->setOpacity(curLoginTextOpacity_ * initOpacity);
        painter->setFont(*FontManager::instance().getFont(24, false));
        painter->setPen(QColor(255,255,255)); //  white

        QString loginText = tr("Login");
        QFontMetrics fm = painter->fontMetrics();
        const int loginTextWidth = fm.width(loginText);
        painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, loginTextWidth),
                          (HEADER_HEIGHT/2 + LOGIN_TEXT_HEIGHT/2)*G_SCALE,
                          loginText);
        painter->restore();

        // Error Text
        painter->setFont(*FontManager::instance().getFont(12, false));
        painter->setPen(FontManager::instance().getErrorRedColor());
        painter->setOpacity(curErrorOpacity_ * initOpacity);
        QRectF rect(WINDOW_MARGIN*G_SCALE, 232*G_SCALE, 180*G_SCALE, 100*G_SCALE);
        painter->drawText(rect, curErrorText_);

        // Emergency Connect Text
        painter->save();
        painter->setPen(FontManager::instance().getBrightBlueColor());
        painter->setOpacity(curEmergencyTextOpacity_ * initOpacity);
        painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, EMERGENCY_CONNECT_TEXT_WIDTH*G_SCALE), EMERGENCY_CONNECT_TEXT_POS_Y*G_SCALE, tr("Emergency Connect is ON"));
        painter->restore();

        // dividers -- bottom buttons
        painter->setOpacity(OPACITY_UNHOVER_DIVIDER * initOpacity);
        const int bottom_button_y = LOGIN_HEIGHT * G_SCALE - settingsButton_->boundingRect().width();
        const int window_center_x_offset = WINDOW_WIDTH/2 * G_SCALE ;

        painter->fillRect(QRect(window_center_x_offset - 30*G_SCALE, bottom_button_y, 2*G_SCALE,  LOGIN_HEIGHT * G_SCALE), Qt::white);
        painter->fillRect(QRect(window_center_x_offset + 29*G_SCALE, bottom_button_y, 2*G_SCALE,  LOGIN_HEIGHT * G_SCALE), Qt::white);

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

    setErrorMessage(ILoginWindow::ERR_MSG_EMPTY); // reset error state

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

#ifdef Q_OS_WIN
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
        setErrorMessage(ILoginWindow::ERR_MSG_INCORRECT_LOGIN1);
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
    QDesktopServices::openUrl(QUrl( QString("https://%1/signup?cpid=app_windows").arg(HardcodedSettings::instance().serverUrl())));
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
    QDesktopServices::openUrl(QUrl( QString("https://%1/forgotpassword").arg(HardcodedSettings::instance().serverUrl())));
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
    onAbstractButtonHoverEnter(emergencyButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Emergency Connect"));
}

void CredentialsWindowItem::onConfigHoverEnter()
{
    onAbstractButtonHoverEnter(configButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "External Config"));
}

void CredentialsWindowItem::onSettingsHoverEnter()
{
    onAbstractButtonHoverEnter(settingsButton_, QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Preferences"));
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
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void CredentialsWindowItem::onLanguageChanged()
{
    twoFactorAuthButton_->recalcBoundingRect();
    forgotPassButton_->recalcBoundingRect();
}

void CredentialsWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MAC)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void CredentialsWindowItem::updatePositions()
{
#ifdef Q_OS_WIN
    closeButton_->setPos((LOGIN_WIDTH - 16 - 16)*G_SCALE, 14*G_SCALE);
    minimizeButton_->setPos((LOGIN_WIDTH - 16 - 16 - 32)*G_SCALE, 14*G_SCALE);
    firewallTurnOffButton_->setPos(8*G_SCALE, 0);
#else
    minimizeButton_->setPos(28*G_SCALE, 8*G_SCALE);
    closeButton_->setPos(8*G_SCALE,8*G_SCALE);
    firewallTurnOffButton_->setPos((LOGIN_WIDTH - 8 - firewallTurnOffButton_->getWidth())*G_SCALE, 0);
#endif

    backButton_->setPos(8*G_SCALE, 36*G_SCALE);

    int bottom_button_y = LOGIN_HEIGHT*G_SCALE - settingsButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE;
    int window_center_x_offset = WINDOW_WIDTH/2*G_SCALE - settingsButton_->boundingRect().width()/2;
    settingsButton_->setPos(window_center_x_offset - 58*G_SCALE, bottom_button_y);

    emergencyButton_->setPos(window_center_x_offset, bottom_button_y);

    configButton_->setPos(window_center_x_offset + 58*G_SCALE, bottom_button_y);

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
    qDebug() << "Credentials::keyPressEvent";
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
                qDebug() << "Credentials::setting focus";
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
    qDebug() << "Credentials::setUsernameFocus()";
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

