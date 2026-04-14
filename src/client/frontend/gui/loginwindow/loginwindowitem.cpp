#include "loginwindowitem.h"

#include <QPainter>
#include <QTextCursor>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QDebug>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"
#include "showingdialogstate.h"
#include "hashutil.h"

extern QWidget *g_mainWindow;

namespace LoginWindow {

const QString EMERGENCY_ICON_ENABLED_PATH  = "login/EMERGENCY_ICON_ENABLED";
const QString EMERGENCY_ICON_DISABLED_PATH = "login/EMERGENCY_ICON_DISABLED";
const QString CONFIG_ICON_PATH             = "login/CONFIG_ICON";
const QString SETTINGS_ICON_PATH           = "login/SETTINGS_ICON";

LoginWindowItem::LoginWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), curError_(LoginWindow::ERR_MSG_EMPTY), curErrorMsg_("")
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    // Header Region:
    backButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backButton_, &IconButton::clicked, this, &LoginWindowItem::onBackClick);

 #if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &LoginWindowItem::onCloseClick);

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &LoginWindowItem::onMinimizeClick);
#else //if Q_OS_MACOS

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &LoginWindowItem::onCloseClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &LoginWindowItem::onMinimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

#endif

    QString firewallOffText = tr("Turn Off Firewall");
    firewallTurnOffButton_ = new FirewallTurnOffButton(std::move(firewallOffText), this);
    connect(firewallTurnOffButton_, &FirewallTurnOffButton::clicked, this, &LoginWindowItem::onFirewallTurnOffClick);

    standardLoginButton_ = new CommonGraphics::TextButton("", FontDescr(14, QFont::Normal), Qt::white, true, this);
    standardLoginButton_->setBigUnderline(true);
    standardLoginButton_->setUnhoverOpacity(OPACITY_FULL);
    standardLoginButton_->setUnhoverOnClick(false);
    connect(standardLoginButton_, &CommonGraphics::TextButton::clicked, this, &LoginWindowItem::onStandardLoginClick);
    standardLoginButton_->quickHide();

    hashedLoginButton_ = new CommonGraphics::TextButton("", FontDescr(14, QFont::Normal), Qt::white, true, this);
    connect(hashedLoginButton_, &CommonGraphics::TextButton::clicked, this, &LoginWindowItem::onHashedLoginClick);
    hashedLoginButton_->setUnhoverOnClick(false);
    hashedLoginButton_->quickHide();

    // Center Region:
    usernameEntry_ = new UsernamePasswordEntry("", false, this);
    passwordEntry_ = new UsernamePasswordEntry("", true, this);
    hashEntry_ = new UsernamePasswordEntry("", false, this);
    hashEntry_->setCustomIcon1("UPLOAD_FILE");
    hashEntry_->setFont(FontManager::instance().getFont(12, QFont::Normal));

    curForgotAnd2FAPosY_ = getForgotAnd2FAPosYDefault();
    connect(&forgotAnd2FAPosYAnimation_, &QVariantAnimation::valueChanged, this, &LoginWindowItem::onForgotAnd2FAPosYChanged);

    curErrorOpacity_ = OPACITY_HIDDEN;
    connect(&errorAnimation_, &QVariantAnimation::valueChanged, this, &LoginWindowItem::onErrorChanged);

    twoFactorAuthButton_ = new CommonGraphics::TextButton("", FontDescr(12, QFont::Normal), Qt::white, true, this);
    connect(twoFactorAuthButton_, &CommonGraphics::TextButton::clicked, this, &LoginWindowItem::onTwoFactorAuthClick);
    twoFactorAuthButton_->quickHide();

    forgotPassButton_ = new CommonGraphics::TextButton("", FontDescr(12, QFont::Normal), Qt::white, true, this);
    connect(forgotPassButton_, &CommonGraphics::TextButton::clicked, this, &LoginWindowItem::onForgotPassClick);
    forgotPassButton_->quickHide();

    loginButton_ = new LoginButton(this);
    connect(loginButton_, &LoginButton::clicked, this, &LoginWindowItem::onLoginClick);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LoginWindowItem::onLanguageChanged);
    onLanguageChanged();

    curEmergencyTextOpacity_ = OPACITY_HIDDEN;
    connect(&emergencyTextAnimation_, &QVariantAnimation::valueChanged, this, &LoginWindowItem::onEmergencyTextTransition);

    connect(usernameEntry_, &UsernamePasswordEntry::textChanged, this, &LoginWindowItem::onUsernamePasswordTextChanged);
    connect(passwordEntry_, &UsernamePasswordEntry::textChanged, this, &LoginWindowItem::onUsernamePasswordTextChanged);
    connect(hashEntry_, &UsernamePasswordEntry::textChanged, this, &LoginWindowItem::onUsernamePasswordTextChanged);
    connect(hashEntry_, &UsernamePasswordEntry::icon1Clicked, this, &LoginWindowItem::onUploadFileClick);

    connect(preferencesHelper, &PreferencesHelper::isDockedModeChanged, this, &LoginWindowItem::onDockedModeChanged);

    usernameEntry_->setOpacityByFactor(OPACITY_FULL);
    passwordEntry_->setOpacityByFactor(OPACITY_FULL);
    hashEntry_->setOpacityByFactor(OPACITY_FULL);

    twoFactorAuthButton_->quickShow();
    forgotPassButton_->quickShow();
    standardLoginButton_->quickShow();
    hashedLoginButton_->quickShow();

    showEditBoxes();
    updatePositions();
}

void LoginWindowItem::setErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    LoginWindow::ErrorMessageInfo info = LoginWindow::getErrorMessageInfo(errorMessageType, errorMessage);
    bool error = info.isError;
    curErrorText_ = info.text;

    curError_ = errorMessageType;
    curErrorMsg_ = errorMessage;

    usernameEntry_->setError(error);
    passwordEntry_->setError(error);

    loginButton_->setError(error);

    int destinationForgotAnd2FAPosY = getForgotAnd2FAPosYDefault();
    double errorOpacity = OPACITY_HIDDEN;
    if (error || !curErrorText_.isEmpty())
    {
        errorOpacity = OPACITY_FULL;
        destinationForgotAnd2FAPosY = getForgotAnd2FAPosYError();
    }

    startAnAnimation<int>(forgotAnd2FAPosYAnimation_, curForgotAnd2FAPosY_,
        destinationForgotAnd2FAPosY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(errorAnimation_, curErrorOpacity_, errorOpacity, ANIMATION_SPEED_FAST);

    update();
}

QRectF LoginWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void LoginWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
    painter->setOpacity(1.0 * initOpacity);
    painter->setFont(FontManager::instance().getFont(24,  QFont::Normal));
    painter->setPen(QColor(255,255,255)); //  white

    QString loginText = tr("Login");
    QFontMetrics fm = painter->fontMetrics();
    const int loginTextWidth = fm.horizontalAdvance(loginText);
    painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, loginTextWidth),
                        (HEADER_HEIGHT/2 + LOGIN_TEXT_HEIGHT/2)*G_SCALE,
                        loginText);
    painter->restore();

    // divider -- between 2FA and forgot password.
    if (twoFactorAuthButton_->getOpacity() != OPACITY_HIDDEN) {
        fm = painter->fontMetrics();

        const int forgot_and_2fa_divider_x =  (WINDOW_MARGIN*G_SCALE
                                              + twoFactorAuthButton_->boundingRect().width() + forgotPassButton_->x()) / 2;
        const int forgot_and_2fa_divider_y = ( 1 + curForgotAnd2FAPosY_ ) * G_SCALE
                                             + (twoFactorAuthButton_->boundingRect().height() - fm.ascent()) / 2;
        painter->fillRect(QRect(forgot_and_2fa_divider_x, forgot_and_2fa_divider_y,
                                1 * G_SCALE, fm.ascent()), Qt::white);
    }

    // Error Text
    painter->setFont(FontManager::instance().getFont(12,  QFont::Normal));
    painter->setPen(FontManager::instance().getErrorRedColor());
    painter->setOpacity(curErrorOpacity_ * initOpacity);
    int add_to_y = 0;
    if (isHashedMode_) {
        add_to_y = -70;
    }
    QRectF rect(WINDOW_MARGIN*G_SCALE, (232+40+add_to_y)*G_SCALE, WINDOW_WIDTH*G_SCALE - loginButton_->boundingRect().width() - 48*G_SCALE, 32*G_SCALE);
    painter->drawText(rect, Qt::AlignVCenter | Qt::TextWordWrap, curErrorText_);

    // Emergency Connect Text
    painter->save();
    painter->setPen(FontManager::instance().getBrightBlueColor());
    painter->setOpacity(curEmergencyTextOpacity_ * initOpacity);
    painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, EMERGENCY_CONNECT_TEXT_WIDTH*G_SCALE), EMERGENCY_CONNECT_TEXT_POS_Y*G_SCALE, tr("Emergency Connect is ON"));
    painter->restore();
}

void LoginWindowItem::resetState()
{
    usernameEntry_->clearActiveState();
    passwordEntry_->clearActiveState();
    hashEntry_->clearActiveState();

    loginButton_->setError(false);
    loginButton_->setEnabled(false);

    setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString()); // reset error state

    curForgotAnd2FAPosY_ = getForgotAnd2FAPosYDefault();
    twoFactorAuthButton_->setPos(WINDOW_MARGIN * G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
            + (WINDOW_MARGIN + FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
}

void LoginWindowItem::setClickable(bool enabled)
{
    const QList<QGraphicsItem*> items = childItems();
    for (QGraphicsItem* child : items) {
        if (auto* clickable = dynamic_cast<ClickableGraphicsObject*>(child))  {
            clickable->setClickable(enabled);
        }
    }
}

void LoginWindowItem::setCurrent2FACode(QString code)
{
    current2FACode_ = code;
}

void LoginWindowItem::onBackClick()
{
    emit backClick();
}

void LoginWindowItem::attemptLogin()
{
    if (!isHashedMode_) {
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
    } else {
        QString hash = hashEntry_->getText();
        if (HashUtil::isValidTruncatedSHA256(hash)) {
            emit loginClick(hash, hash, current2FACode_);
        } else {
            setErrorMessage(LoginWindow::ERR_MSG_INCORRECT_LOGIN1, QString());
        }
    }
}

void LoginWindowItem::showEditBoxes()
{
    if (!isHashedMode_) {
        hashEntry_->setClickable(false);
        hashEntry_->hide();

        usernameEntry_->setClickable(true);
        passwordEntry_->setClickable(true);
        usernameEntry_->show();
        passwordEntry_->show();
    } else {
        usernameEntry_->setClickable(false);
        passwordEntry_->setClickable(false);
        usernameEntry_->hide();
        passwordEntry_->hide();

        hashEntry_->setClickable(true);
        hashEntry_->show();
    }
}

void LoginWindowItem::onLoginClick()
{
    attemptLogin();
}

void LoginWindowItem::onCloseClick()
{
    emit closeClick();
}

void LoginWindowItem::onMinimizeClick()
{
    emit minimizeClick();
}

void LoginWindowItem::onTwoFactorAuthClick()
{
    QString username;
    QString password;
    if (!isHashedMode_) {
        username = usernameEntry_->getText();
        password = passwordEntry_->getText();
    } else {
        username = hashEntry_->getText();
        password = hashEntry_->getText();
    }
    twoFactorAuthButton_->unhover();
    emit twoFactorAuthClick(username, password);
}

void LoginWindowItem::onForgotPassClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/forgotpassword").arg(HardcodedSettings::instance().windscribeServerUrl())));
}

void LoginWindowItem::onForgotAnd2FAPosYChanged(const QVariant &value)
{
    curForgotAnd2FAPosY_ = value.toInt();

    twoFactorAuthButton_->setPos(WINDOW_MARGIN*G_SCALE, curForgotAnd2FAPosY_*G_SCALE);
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
        + (WINDOW_MARGIN+FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_*G_SCALE);

    update();
}

void LoginWindowItem::onErrorChanged(const QVariant &value)
{
    curErrorOpacity_ = value.toDouble();
    update();
}

void LoginWindowItem::onEmergencyTextTransition(const QVariant &value)
{
    curEmergencyTextOpacity_ = value.toDouble();
    update();
}

void LoginWindowItem::onUsernamePasswordTextChanged(const QString & /*text*/)
{
    loginButton_->setEnabled(isUsernameAndPasswordValid());
}

void LoginWindowItem::onLanguageChanged()
{
    twoFactorAuthButton_->setText(tr("2FA Code"));
    twoFactorAuthButton_->recalcBoundingRect();
    forgotPassButton_->setText(tr("Forgot password?"));
    forgotPassButton_->recalcBoundingRect();
    standardLoginButton_->setText(tr("Standard"));
    standardLoginButton_->recalcBoundingRect();
    hashedLoginButton_->setText(tr("Hashed"));
    hashedLoginButton_->recalcBoundingRect();
    usernameEntry_->setDescription(tr("Username"));
    usernameEntry_->setPlaceholderText(tr("Enter username"));
    passwordEntry_->setDescription(tr("Password"));
    passwordEntry_->setPlaceholderText(tr("Enter password"));
    hashEntry_->setDescription(tr("Hash"));
    hashEntry_->setPlaceholderText(tr("Account Hash or upload file"));
    setErrorMessage(curError_, curErrorMsg_);
    updatePositions();
}

void LoginWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MACOS)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void LoginWindowItem::updatePositions()
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

    int y_offs = 40 * G_SCALE;
    usernameEntry_->setPos(0, USERNAME_POS_Y_VISIBLE*G_SCALE + y_offs);
    passwordEntry_->setPos(0, PASSWORD_POS_Y_VISIBLE*G_SCALE + y_offs);
    hashEntry_->setPos(0, USERNAME_POS_Y_VISIBLE*G_SCALE + y_offs);

    if (!isHashedMode_) {
        loginButton_->setPos((LOGIN_WIDTH - 32 - WINDOW_MARGIN)*G_SCALE, LOGIN_BUTTON_POS_Y*G_SCALE + y_offs);
    } else {
        loginButton_->setPos((LOGIN_WIDTH - 32 - WINDOW_MARGIN)*G_SCALE, (LOGIN_BUTTON_POS_Y-70)*G_SCALE + y_offs);
    }

    int x_center = LOGIN_WIDTH * G_SCALE / 2;
    standardLoginButton_->setPos(x_center - standardLoginButton_->getWidth() - 10*G_SCALE, 90*G_SCALE);
    hashedLoginButton_->setPos(x_center + 10*G_SCALE, 90*G_SCALE);


    twoFactorAuthButton_->setPos(WINDOW_MARGIN * G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    twoFactorAuthButton_->recalcBoundingRect();
    forgotPassButton_->setPos(twoFactorAuthButton_->boundingRect().width()
        + (WINDOW_MARGIN + FORGOT_POS_X_SPACING)*G_SCALE, curForgotAnd2FAPosY_ * G_SCALE);
    forgotPassButton_->recalcBoundingRect();
}

void LoginWindowItem::keyPressEvent(QKeyEvent *event)
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

bool LoginWindowItem::sceneEvent(QEvent *event)
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
                if (!isHashedMode_) {
                    usernameEntry_->setFocus();
                } else {
                    hashEntry_->setFocus();
                }
                return true;
            }
        }
    }

    QGraphicsItem::sceneEvent(event);
    return false;
}

void LoginWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
    usernameEntry_->updateScaling();
    passwordEntry_->updateScaling();
    hashEntry_->updateScaling();
}

void LoginWindowItem::setUsernameFocus()
{
    if (!isHashedMode_) {
        usernameEntry_->setFocus();
    } else {
        hashEntry_->setFocus();
    }
}

int LoginWindowItem::centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

bool LoginWindowItem::isUsernameAndPasswordValid() const
{
    if (!isHashedMode_) {
        return !usernameEntry_->getText().isEmpty() && !passwordEntry_->getText().isEmpty();
    } else {
        return !hashEntry_->getText().isEmpty();
    }
}

int LoginWindowItem::getForgotAnd2FAPosYDefault() const
{
    if (!isHashedMode_) {
        return 280;
    } else {
        return 210;
    }
}

int LoginWindowItem::getForgotAnd2FAPosYError() const
{
    if (!isHashedMode_) {
        return 306;
    } else {
        return 236;
    }
}

void LoginWindowItem::onFirewallTurnOffClick()
{
    firewallTurnOffButton_->animatedHide();
    emit firewallTurnOffClick();
}

void LoginWindowItem::onStandardLoginClick()
{
    isHashedMode_ = false;
    standardLoginButton_->setBigUnderline(true);
    standardLoginButton_->setUnhoverOpacity(OPACITY_FULL);
    hashedLoginButton_->setBigUnderline(false);
    hashedLoginButton_->setUnhoverOpacity(OPACITY_SIXTY);
    hashedLoginButton_->setCurrentOpacity(OPACITY_SIXTY);

    showEditBoxes();
    update();
    updatePositions();
    resetState();
    setUsernameFocus();
}

void LoginWindowItem::onHashedLoginClick()
{
    isHashedMode_ = true;
    standardLoginButton_->setBigUnderline(false);
    standardLoginButton_->setUnhoverOpacity(OPACITY_SIXTY);
    standardLoginButton_->setCurrentOpacity(OPACITY_SIXTY);
    hashedLoginButton_->setBigUnderline(true);
    hashedLoginButton_->setUnhoverOpacity(OPACITY_FULL);

    showEditBoxes();
    update();
    resetState();
    updatePositions();
    setUsernameFocus();
}

void LoginWindowItem::onUploadFileClick()
{
    // We do it through QTimer, otherwise the dialog window is blocked
    QTimer::singleShot(0, this, [this]() {
        ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
        const QString filename = QFileDialog::getOpenFileName(g_mainWindow, tr("Select file for hash"), "", tr("All Files (*.*)"));
        ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);
        if (filename.isEmpty()) {
            return;
        }
        hashEntry_->setText(HashUtil::getTruncatedSHA256(filename));
    });
}

void LoginWindowItem::setFirewallTurnOffButtonVisibility(bool visible)
{
    firewallTurnOffButton_->setActive(visible);
}

} // namespace LoginWindow

