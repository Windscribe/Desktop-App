#include "signupwindowitem.h"

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
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QFileDialog>
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

SignupWindowItem::SignupWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent)
{
    WS_ASSERT(preferencesHelper);
    setFlag(QGraphicsItem::ItemIsFocusable);

    curHeight_ = 2*LOGIN_HEIGHT*G_SCALE;

    // Header Region:
    backButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backButton_, &IconButton::clicked, this, &SignupWindowItem::backClick);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &SignupWindowItem::closeClick);

    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &SignupWindowItem::minimizeClick);
#else //if Q_OS_MACOS
    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &SignupWindowItem::closeClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &SignupWindowItem::minimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);
#endif


    standardLoginButton_ = new CommonGraphics::TextButton("", FontDescr(14, QFont::Normal), Qt::white, true, this);
    standardLoginButton_->setBigUnderline(true);
    standardLoginButton_->setUnhoverOpacity(OPACITY_FULL);
    standardLoginButton_->setUnhoverOnClick(false);
    connect(standardLoginButton_, &CommonGraphics::TextButton::clicked, this, &SignupWindowItem::onStandardLoginClick);
    standardLoginButton_->quickHide();

    hashedLoginButton_ = new CommonGraphics::TextButton("", FontDescr(14, QFont::Normal), Qt::white, true, this);
    connect(hashedLoginButton_, &CommonGraphics::TextButton::clicked, this, &SignupWindowItem::onHashedLoginClick);
    hashedLoginButton_->setUnhoverOnClick(false);
    hashedLoginButton_->quickHide();

    // Center Region:
    usernameEntry_ = new UsernamePasswordEntry("", false, this);
    usernameEntry_->setOpacityByFactor(OPACITY_FULL);
    connect(usernameEntry_, &UsernamePasswordEntry::textChanged, this, [this] {
        usernameEntry_->setError(false);
    });

    passwordEntry_ = new UsernamePasswordEntry("", true, this);
    passwordEntry_->setOpacityByFactor(OPACITY_FULL);
    connect(passwordEntry_, &UsernamePasswordEntry::textChanged, this, [this] {
        passwordEntry_->setError(false);
    });

    passwordHint_ = new CommonGraphics::TextItem(this, "", FontDescr(11, QFont::Normal), Qt::white, (WINDOW_WIDTH- 36)*G_SCALE);

    hashEntry_ = new UsernamePasswordEntry("", false, this);
    hashEntry_->setCustomIcon1("REFRESH_ICON");
    hashEntry_->setCustomIcon2("UPLOAD_FILE");
    hashEntry_->setOpacityByFactor(OPACITY_FULL);
    hashEntry_->setFont(FontManager::instance().getFont(12, QFont::Normal));
    connect(hashEntry_, &UsernamePasswordEntry::textChanged, this, [this] {
        hashEntry_->setError(false);
    });
    connect(hashEntry_, &UsernamePasswordEntry::icon1Clicked, this, &SignupWindowItem::onGenerateHashCodeClick);
    connect(hashEntry_, &UsernamePasswordEntry::icon2Clicked, this, &SignupWindowItem::onUploadFileClick);

    hashHint_ = new CommonGraphics::TextItem(this, "", FontDescr(11, QFont::Normal), Qt::white, (WINDOW_WIDTH- 36)*G_SCALE);


    emailEntry_ = new UsernamePasswordEntry("", false, this);
    emailEntry_->setOpacityByFactor(OPACITY_FULL);
    connect(emailEntry_, &UsernamePasswordEntry::textChanged, this, [this] {
        emailEntry_->setError(false);
    });

    emailHint_ = new CommonGraphics::TextItem(this, "", FontDescr(11, QFont::Normal), Qt::white, (WINDOW_WIDTH- 36)*G_SCALE);

    btnVoucherCode_ = new CommonGraphics::TextWrapIconButton(0, "", "ARROW_DOWN_LOCATIONS", this, true);
    btnVoucherCode_->setFont(FontDescr(12, QFont::Normal));
    btnVoucherCode_->setCursor(Qt::PointingHandCursor);
    btnVoucherCode_->setMaxWidth((WINDOW_WIDTH- 30)*G_SCALE);
    connect(btnVoucherCode_, &CommonGraphics::TextWrapIconButton::clicked, this, &SignupWindowItem::onVoucherCodeClick);
    voucherEntry_ = new UsernamePasswordEntry("", false, this, false);
    voucherEntry_->setOpacityByFactor(OPACITY_FULL);
    voucherEntry_->hide();

    btnReferred_ = new CommonGraphics::TextWrapIconButton(0, "", "ARROW_DOWN_LOCATIONS", this, true);
    btnReferred_->setFont(FontDescr(12, QFont::Normal));
    btnReferred_->setCursor(Qt::PointingHandCursor);
    btnReferred_->setMaxWidth((WINDOW_WIDTH- 30)*G_SCALE);
    connect(btnReferred_, &CommonGraphics::TextWrapIconButton::clicked, this, &SignupWindowItem::onReferredClick);
    referredEntry_ = new UsernamePasswordEntry("", false, this, false);
    referredEntry_->setOpacityByFactor(OPACITY_FULL);
    referredEntry_->hide();

    errorHint_ = new CommonGraphics::TextItem(this, "", FontDescr(11, QFont::Normal), Qt::red, (WINDOW_WIDTH- 36)*G_SCALE);


    continueButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kWelcome, 121, 38, 19);
    continueButton_->setFont(FontDescr(15, QFont::Medium));
    continueButton_->setWidth(WINDOW_WIDTH - 36);
    connect(continueButton_, &CommonGraphics::BubbleButton::clicked, this, &SignupWindowItem::onContinueClick);
    continueButton_->setClickable(true);
    continueButton_->show();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SignupWindowItem::onLanguageChanged);
    onLanguageChanged();

    standardLoginButton_->quickShow();
    hashedLoginButton_->quickShow();
    showEditBoxes();
    updatePositions();
}

QRectF SignupWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, curHeight_);
}

void SignupWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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

    QRectF rcBottomRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, curHeight_ - HEADER_HEIGHT*G_SCALE);

    QColor darkblue = FontManager::instance().getDarkBlueColor();
    painter->setBrush(darkblue);
    painter->setPen(darkblue);
    painter->drawRoundedRect(rcBottomRect, 9*G_SCALE, 9*G_SCALE);

    // We don't actually want the upper corners of the bottom rect to be rounded.  Fill them in
    painter->fillRect(QRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, 9*G_SCALE), darkblue);

    // Sign Up Text
    painter->save();
    painter->setOpacity(initOpacity);
    painter->setFont(FontManager::instance().getFont(22,  QFont::Normal));
    painter->setPen(QColor(255,255,255)); //  white

    QString text = tr("Sign Up");
    QFontMetrics fm = painter->fontMetrics();
    const int loginTextWidth = fm.horizontalAdvance(text);
    painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, loginTextWidth),
                      (HEADER_HEIGHT/2 + LOGIN_TEXT_HEIGHT/2)*G_SCALE,
                      text);
    painter->restore();

}

void SignupWindowItem::updateScaling()
{
    // These items store pixel-valued widths that must be recomputed for the new scale
    // before ScalableGraphicsObject::updateScaling() propagates to children and
    // triggers their recalcBoundingRect() / recalcWidth() calls.
    const int hintMaxWidth = (WINDOW_WIDTH - 36) * G_SCALE;
    passwordHint_->setMaxWidth(hintMaxWidth);
    hashHint_->setMaxWidth(hintMaxWidth);
    emailHint_->setMaxWidth(hintMaxWidth);
    errorHint_->setMaxWidth(hintMaxWidth);

    const int btnMaxWidth = (WINDOW_WIDTH - 30) * G_SCALE;
    btnVoucherCode_->setMaxWidth(btnMaxWidth);
    btnReferred_->setMaxWidth(btnMaxWidth);

    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void SignupWindowItem::setClickable(bool enabled)
{
    const QList<QGraphicsItem*> items = childItems();
    for (QGraphicsItem* child : items) {
        if (auto* clickable = dynamic_cast<ClickableGraphicsObject*>(child))  {
            clickable->setClickable(enabled);
        }
    }
}

void SignupWindowItem::resetState()
{
    usernameEntry_->clearActiveState();
    passwordEntry_->clearActiveState();
    hashEntry_->clearActiveState();
    emailEntry_->clearActiveState();
    voucherEntry_->clearActiveState();
    referredEntry_->clearActiveState();
    setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString()); // reset error state
}

void SignupWindowItem::setUsernameFocus()
{
    if (!isHashedMode_) {
        usernameEntry_->setFocus();
    } else {
        hashEntry_->setFocus();
    }
}

void SignupWindowItem::setErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    LoginWindow::ErrorMessageInfo info = LoginWindow::getErrorMessageInfo(errorMessageType, errorMessage);
    bool error = info.isError;
    curErrorText_ = info.text;

    // Update UI elements with error state
    usernameEntry_->setError(error);
    passwordEntry_->setError(error);
    emailEntry_->setError(error);
    hashEntry_->setError(error);

    // Show or hide error message
    if (error || !curErrorText_.isEmpty())
    {
        isError_ = true;
        errorHint_->setText(curErrorText_);
        errorHint_->show();
    }
    else
    {
        isError_ = false;
        errorHint_->hide();
    }

    updatePositions();
    update();
}

void SignupWindowItem::onContinueClick()
{
    if (verifyInputs()) {
        if (!isHashedMode_) {
            emit signupClick(usernameEntry_->getText(), passwordEntry_->getText(), emailEntry_->getText(),
                             voucherEntry_->getText(), referredEntry_->getText());
        } else {
            emit signupClick(hashEntry_->getText(), hashEntry_->getText(), QString(),
                             voucherEntry_->getText(), referredEntry_->getText());
        }
    }
}

void SignupWindowItem::onLanguageChanged()
{
    standardLoginButton_->setText(tr("Standard"));
    standardLoginButton_->recalcBoundingRect();
    hashedLoginButton_->setText(tr("Hashed"));
    hashedLoginButton_->recalcBoundingRect();
    usernameEntry_->setDescription(tr("Username"));
    usernameEntry_->setPlaceholderText(tr("Enter username"));
    passwordEntry_->setDescription(tr("Password"));
    passwordEntry_->setPlaceholderText(tr("Enter password"));
    passwordHint_->setText(tr("8 or more characters with at least one uppercase and lowercase(ie. \"Hello1234\", \"Solyanka\")."));
    emailEntry_->setDescription(tr("Email (Optional)"));
    emailEntry_->setPlaceholderText(tr("Enter email"));
    emailHint_->setText(tr("For password recovery, updates & promo only. No spam."));
    btnVoucherCode_->setText(tr("Voucher Code?"));
    voucherEntry_->setPlaceholderText(tr("Voucher code (optional)"));
    btnReferred_->setText(tr("Referred By Someone?"));
    referredEntry_->setPlaceholderText(tr("Referring username (optional)"));
    errorHint_->setText(curErrorText_);
    continueButton_->setText(tr("Continue"));

    hashEntry_->setDescription(tr("Hash"));
    hashEntry_->setPlaceholderText(tr("Generate or browse file"));
    hashHint_->setText(tr("No personal info required. Account recovery is impossible.\nIf you lose your account hash, it's gone forever and support cannot help you recover it."));

    updatePositions();
}

void SignupWindowItem::onStandardLoginClick()
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

void SignupWindowItem::onHashedLoginClick()
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

void SignupWindowItem::onVoucherCodeClick()
{
    if (!isVoucherCodeExpanded_) {
        isVoucherCodeExpanded_ = true;
        btnVoucherCode_->setImagePath("ARROW_UP");
        voucherEntry_->setClickable(true);
        voucherEntry_->show();

    } else {
        isVoucherCodeExpanded_ = false;
        btnVoucherCode_->setImagePath("ARROW_DOWN_LOCATIONS");
        voucherEntry_->setClickable(false);
        voucherEntry_->hide();
    }
    updatePositions();
}

void SignupWindowItem::onReferredClick()
{
    if (!isReferredExpanded_) {
        isReferredExpanded_ = true;
        btnReferred_->setImagePath("ARROW_UP");
        referredEntry_->setClickable(true);
        referredEntry_->show();

    } else {
        isReferredExpanded_ = false;
        btnReferred_->setImagePath("ARROW_DOWN_LOCATIONS");
        referredEntry_->setClickable(false);
        referredEntry_->hide();
    }
    updatePositions();
}

void SignupWindowItem::onGenerateHashCodeClick()
{
    QString randomHash = HashUtil::generateRandomTruncatedHash();
    hashEntry_->setText(randomHash);
}

void SignupWindowItem::onUploadFileClick()
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

void SignupWindowItem::updatePositions()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_->setPos((LOGIN_WIDTH - 16 - 16)*G_SCALE, 14*G_SCALE);
    minimizeButton_->setPos((LOGIN_WIDTH - 16 - 16 - 32)*G_SCALE, 14*G_SCALE);
#else
    minimizeButton_->setPos(28*G_SCALE, 8*G_SCALE);
    closeButton_->setPos(8*G_SCALE,8*G_SCALE);
#endif

    backButton_->setPos(16*G_SCALE, 28*G_SCALE);

    int x_center = LOGIN_WIDTH * G_SCALE / 2;
    standardLoginButton_->setPos(x_center - standardLoginButton_->getWidth() - 10*G_SCALE, 90*G_SCALE);
    hashedLoginButton_->setPos(x_center + 10*G_SCALE, 90*G_SCALE);


    int y_offs = 132 * G_SCALE;
    usernameEntry_->setPos(0, y_offs);
    hashEntry_->setPos(0, y_offs);
    y_offs += 54 * G_SCALE;

    if (!isHashedMode_) {
        y_offs += 18 * G_SCALE;
        passwordEntry_->setPos(0, y_offs);
        y_offs += 54 * G_SCALE;
        passwordHint_->setPos(16 * G_SCALE, y_offs);
        y_offs += passwordHint_->getHeight() + 20 * G_SCALE;
        emailEntry_->setPos(0, y_offs);
        y_offs += 54 * G_SCALE;
        emailHint_->setPos(16 * G_SCALE, y_offs);
        y_offs += emailHint_->getHeight() + 20 * G_SCALE;
    } else {
        y_offs += 4 * G_SCALE;
        hashHint_->setPos(16 * G_SCALE, y_offs);
        y_offs += hashHint_->getHeight() + 20 * G_SCALE;
    }

    btnVoucherCode_->setPos(16 * G_SCALE, y_offs);
    y_offs += btnVoucherCode_->getHeight() + 4 * G_SCALE;
    if (isVoucherCodeExpanded_) {
        voucherEntry_->setPos(0, y_offs);
        y_offs += 44 * G_SCALE;
    }

    btnReferred_->setPos(16 * G_SCALE, y_offs);
    y_offs += btnReferred_->getHeight() + 4 * G_SCALE;
    if (isReferredExpanded_) {
        referredEntry_->setPos(0, y_offs);
        y_offs += 44 * G_SCALE;
    }

    if (isError_) {
        y_offs += 4 * G_SCALE;
        errorHint_->setPos(16 * G_SCALE, y_offs);
        y_offs += errorHint_->getHeight() + 16 * G_SCALE;

    }

    y_offs += 6 * G_SCALE;
    continueButton_->setPos((LOGIN_WIDTH*G_SCALE - continueButton_->boundingRect().width()) / 2, y_offs);
    y_offs += 20 * G_SCALE;

    prepareGeometryChange();

    curHeight_ = y_offs + continueButton_->boundingRect().height();
    emit heightChanged();
}

int SignupWindowItem::centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

void SignupWindowItem::showEditBoxes()
{
    if (!isHashedMode_) {
        hashEntry_->setClickable(false);
        hashEntry_->hide();
        hashHint_->hide();

        usernameEntry_->setClickable(true);
        passwordEntry_->setClickable(true);
        usernameEntry_->show();
        passwordEntry_->show();
        passwordHint_->show();
        emailEntry_->show();
        emailHint_->show();
    } else {
        usernameEntry_->setClickable(false);
        passwordEntry_->setClickable(false);
        usernameEntry_->hide();
        passwordEntry_->hide();
        passwordHint_->hide();
        emailEntry_->hide();
        emailHint_->hide();

        hashEntry_->setClickable(true);
        hashEntry_->show();
        hashHint_->show();
    }
}

bool SignupWindowItem::verifyInputs()
{
    isError_ = false;
    if (!isHashedMode_) {
        QString username = usernameEntry_->getText();
        QString password = passwordEntry_->getText();
        QString email = emailEntry_->getText();
        if (username.length() < 3) {
            isError_ = true;
            usernameEntry_->setError(true);
            errorHint_->setText(tr("Username must be at least 3 characters"));
        } else if (isEmailValid(username)) {
            isError_ = true;
            usernameEntry_->setError(true);
            errorHint_->setText(tr("Your username should not be an email address."));
        } else if (!isPasswordValid(password)) {
            isError_ = true;
            passwordEntry_->setError(true);
            errorHint_->setText(tr("8 or more characters with at least one uppercase and lowercase(ie. \"Hello1234\", \"Solyanka\")."));
        } else if (!isEmailValid(email)) {
            isError_ = true;
            emailEntry_->setError(true);
            errorHint_->setText(tr("Invalid email"));
        }
    } else {
        if (!HashUtil::isValidTruncatedSHA256(hashEntry_->getText()))
        {
            isError_ = true;
            hashEntry_->setError(true);
            errorHint_->setText(tr("Invalid hash"));
        }
    }
    if (isError_) {
        errorHint_->show();
    } else {
        errorHint_->hide();
    }

    updatePositions();
    return !isError_;
}

bool SignupWindowItem::isPasswordValid(const QString &password)
{
    if (password.length() < 8)
        return false;
    bool hasUpper = false;
    bool hasLower = false;
    for (const QChar &c : password) {
        if (c.isUpper())
            hasUpper = true;
        if (c.isLower())
            hasLower = true;
        if (hasUpper && hasLower)
            return true;
    }
    return hasUpper && hasLower;
}

bool SignupWindowItem::isEmailValid(const QString &email)
{
    if (email.isEmpty())
        return true;
    // Practical format: local@domain.tld (letters, digits, ._-+ in local; domain with at least one dot)
    static const QRegularExpression emailRegex(
        QStringLiteral("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"));
    return emailRegex.match(email).hasMatch();
}

} // namespace LoginWindow

