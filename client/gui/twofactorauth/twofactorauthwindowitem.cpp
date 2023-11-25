#include "twofactorauthwindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"

namespace TwoFactorAuthWindow
{
TwoFactorAuthWindowItem::TwoFactorAuthWindowItem(QGraphicsObject *parent,
                                                 PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curTextOpacity_ = OPACITY_FULL;
    curEscTextOpacity_ = OPACITY_FULL;

    okButton_ = new TwoFactorAuthOkButton(this);
    okButton_->setClickable(false);
    connect(okButton_, &TwoFactorAuthOkButton::clicked, this, &TwoFactorAuthWindowItem::onButtonClicked);

    escButton_ = new CommonGraphics::EscapeButton(this);
    connect(escButton_, &CommonGraphics::EscapeButton::clicked, this, &TwoFactorAuthWindowItem::onEscClicked);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &TwoFactorAuthWindowItem::closeClick);

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &TwoFactorAuthWindowItem::minimizeClick);
#else //if Q_OS_MAC
    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &TwoFactorAuthWindowItem::closeClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &TwoFactorAuthWindowItem::minimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);
#endif

    curErrorOpacity_ = OPACITY_HIDDEN;
    connect(&errorAnimation_, &QVariantAnimation::valueChanged, this, &TwoFactorAuthWindowItem::onErrorChanged);

    curCodeEntryOpacity_ = OPACITY_HIDDEN;
    connect(&codeEntryOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TwoFactorAuthWindowItem::onCodeEntryOpacityChanged);

    codeEntry_ = new LoginWindow::UsernamePasswordEntry(QString(), false, this);
    connect(codeEntry_, &LoginWindow::UsernamePasswordEntry::textChanged, this, &TwoFactorAuthWindowItem::onCodeTextChanged);
    codeEntry_->setClickable(true);

    connect(preferencesHelper, &PreferencesHelper::isDockedModeChanged, this, &TwoFactorAuthWindowItem::onDockedModeChanged);

    updatePositions();
}

QRectF TwoFactorAuthWindowItem::boundingRect() const
{
    return QRect(0,0,LOGIN_WIDTH*G_SCALE,LOGIN_HEIGHT*G_SCALE);
}

void TwoFactorAuthWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    const qreal initialOpacity = painter->opacity();

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    const int pushDownSquare = 0;
#else
    const int pushDownSquare = 5;
#endif

    QRectF rcTopRect(0, 0, WINDOW_WIDTH*G_SCALE, HEADER_HEIGHT*G_SCALE);
    QRectF rcBottomRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE,
                        (LOGIN_HEIGHT - HEADER_HEIGHT)*G_SCALE);
    QColor black = FontManager::instance().getMidnightColor();
    QColor darkblue = FontManager::instance().getDarkBlueColor();

#ifndef Q_OS_WIN // round background
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(rcTopRect, 5 * G_SCALE, 5 * G_SCALE);

    painter->setBrush(darkblue);
    painter->setPen(darkblue);
    painter->drawRoundedRect(rcBottomRect.adjusted(0, 5 * G_SCALE, 0, 0), 5 * G_SCALE, 5 * G_SCALE);
#endif

    // Square background
    painter->fillRect(rcTopRect.adjusted(0, pushDownSquare, 0, 0), black);
    painter->fillRect(rcBottomRect.adjusted(0, 0, 0, -pushDownSquare * 2), darkblue);

    // Icon
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    QSharedPointer<IndependentPixmap> pixmap =
        ImageResourcesSvg::instance().getIndependentPixmap("login/TWOFA_ICON");
    pixmap->draw((WINDOW_WIDTH / 2 - 20)*G_SCALE, (HEADER_HEIGHT - 40)*G_SCALE, painter);

    // Title
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(16, true));
    painter->setPen(Qt::white);
    painter->drawText(QRect(0, 87 * G_SCALE, WINDOW_WIDTH * G_SCALE, 20 * G_SCALE),
                      Qt::AlignCenter, tr("Two-factor Auth"));

    // Message
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(14, false));
    painter->setPen(Qt::white);
    painter->drawText(QRect(((WINDOW_WIDTH - CODE_ENTRY_WIDTH) / 2)*G_SCALE, 123*G_SCALE,
                            CODE_ENTRY_WIDTH*G_SCALE, 36*G_SCALE),
                      Qt::AlignCenter | Qt::TextWordWrap,
                      tr("Use your app to get an authentication code, and enter it below"));

    // Error Text
    if (!curErrorText_.isEmpty()) {
        painter->setFont(*FontManager::instance().getFont(12, false));
        painter->setPen(FontManager::instance().getErrorRedColor());
        painter->setOpacity(curErrorOpacity_ * initialOpacity);
        QRectF rect(WINDOW_MARGIN*G_SCALE, 270*G_SCALE, (WINDOW_WIDTH- WINDOW_MARGIN*2)*G_SCALE,
                    16*G_SCALE);
        painter->drawText(rect, Qt::AlignCenter, curErrorText_);
    }
}

void TwoFactorAuthWindowItem::clearCurrentCredentials()
{
    savedUsername_.clear();
    savedPassword_.clear();
}

void TwoFactorAuthWindowItem::setCurrentCredentials(const QString &username,
                                                    const QString &password)
{
    savedUsername_ = username;
    savedPassword_ = password;
}

void TwoFactorAuthWindowItem::setLoginMode(bool is_login_mode)
{
    okButton_->setButtonType(is_login_mode ? TwoFactorAuthOkButton::BUTTON_TYPE_LOGIN
        : TwoFactorAuthOkButton::BUTTON_TYPE_ADD);
}

void TwoFactorAuthWindowItem::setErrorMessage(ERROR_MESSAGE_TYPE errorMessage)
{
    bool error = false;

    switch (errorMessage)
    {
    case ERR_MSG_EMPTY:
        curErrorText_.clear();
        break;
    case ERR_MSG_NO_CODE:
        curErrorText_ = tr("Please provide a 2FA code");
        break;
    case ERR_MSG_INVALID_CODE:
        error = true;
        curErrorText_ = tr("Invalid 2FA code, please try again");
        break;
    default:
        WS_ASSERT(false);
        break;
    }

    codeEntry_->setError(error);

    double errorOpacity = OPACITY_HIDDEN;
    if (error || !curErrorText_.isEmpty())
        errorOpacity = OPACITY_FULL;
    startAnAnimation<double>(errorAnimation_, curErrorOpacity_, errorOpacity, ANIMATION_SPEED_FAST);
    update();
}

void TwoFactorAuthWindowItem::resetState()
{
    curCodeEntryOpacity_ = OPACITY_HIDDEN;
    codeEntry_->setOpacityByFactor(curCodeEntryOpacity_);
    codeEntry_->clearActiveState();
    okButton_->setClickable(!codeEntry_->getText().isEmpty());
}

void TwoFactorAuthWindowItem::setClickable(bool isClickable)
{
    escButton_->setClickable(isClickable);
    okButton_->setClickable(isClickable && !codeEntry_->getText().isEmpty());

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_->setClickable(isClickable);
    minimizeButton_->setClickable(isClickable);
#endif

    codeEntry_->setClickable(isClickable);
    if (isClickable) {
        startAnAnimation<double>(codeEntryOpacityAnimation_, curCodeEntryOpacity_,
                                 OPACITY_FULL, ANIMATION_SPEED_SLOW);
        QTimer::singleShot(ANIMATION_SPEED_SLOW, [this]() { codeEntry_->setFocus(); });
    }
}

void TwoFactorAuthWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    okButton_->setFont(FontDescr(14, false));
    codeEntry_->updateScaling();
    updatePositions();
}

void TwoFactorAuthWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        codeEntry_->clearActiveState();
        emit escapeClick();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        onButtonClicked();
    }
}

void TwoFactorAuthWindowItem::onCodeTextChanged(const QString &text)
{
    okButton_->setClickable(!text.isEmpty());
}

void TwoFactorAuthWindowItem::onCodeEntryOpacityChanged(const QVariant &value)
{
    curCodeEntryOpacity_ = value.toDouble();
    codeEntry_->setOpacityByFactor(curCodeEntryOpacity_);
}

void TwoFactorAuthWindowItem::onErrorChanged(const QVariant &value)
{
    curErrorOpacity_ = value.toDouble();
    update();
}

void TwoFactorAuthWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MAC)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void TwoFactorAuthWindowItem::onEscClicked()
{
    resetState();
    emit escapeClick();
}

void TwoFactorAuthWindowItem::onButtonClicked()
{
    if (codeEntry_->getText().isEmpty()) {
        setErrorMessage(ERR_MSG_NO_CODE);
        return;
    }
    setErrorMessage(ERR_MSG_EMPTY);
    switch (okButton_->buttonType()) {
    case TwoFactorAuthOkButton::BUTTON_TYPE_ADD:
        emit addClick(codeEntry_->getText());
        break;
    case TwoFactorAuthOkButton::BUTTON_TYPE_LOGIN:
        emit loginClick(savedUsername_, savedPassword_, codeEntry_->getText());
        break;
    default:
        WS_ASSERT(false);
    }
}

void TwoFactorAuthWindowItem::onTextOpacityChange(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void TwoFactorAuthWindowItem::onEscTextOpacityChange(const QVariant &value)
{
    curEscTextOpacity_ = value.toDouble();
    update();
}

void TwoFactorAuthWindowItem::updatePositions()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_->setPos((LOGIN_WIDTH - 16 - WINDOW_MARGIN)*G_SCALE, 14*G_SCALE);
    minimizeButton_->setPos((LOGIN_WIDTH - 16 - WINDOW_MARGIN -32)*G_SCALE, 14*G_SCALE);
    escButton_->setPos(WINDOW_MARGIN*G_SCALE, WINDOW_MARGIN*G_SCALE);
#else
    minimizeButton_->setPos(28*G_SCALE, 8*G_SCALE);
    closeButton_->setPos(    8*G_SCALE, 8*G_SCALE);
    escButton_->setPos((WINDOW_WIDTH - WINDOW_MARGIN - escButton_->getSize())*G_SCALE,
                        WINDOW_MARGIN*G_SCALE);
#endif

    okButton_->setPos(WINDOW_WIDTH/2*G_SCALE - okButton_->boundingRect().width()/2,
                      OK_BUTTON_POS_Y*G_SCALE);

    codeEntry_->setPos(((WINDOW_WIDTH- CODE_ENTRY_WIDTH)/2)*G_SCALE, CODE_ENTRY_POS_Y*G_SCALE);
    codeEntry_->setWidth(CODE_ENTRY_WIDTH);
}
}  // namespace TwoFactorAuthWindow
