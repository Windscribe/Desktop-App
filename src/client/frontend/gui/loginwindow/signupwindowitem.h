#pragma once

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include "backend/backend.h"
#include "commongraphics/resizablewindow.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/textbutton.h"
#include "commongraphics/textwrapiconbutton.h"
#include "commongraphics/textitem.h"
#include "usernamepasswordentry.h"
#include "loginwindowtypes.h"

namespace LoginWindow {

class SignupWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit SignupWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;
    void setClickable(bool enabled);
    void resetState();
    void setUsernameFocus();
    void setErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage = QString());

signals:
    void minimizeClick();
    void closeClick();
    void backClick();
    void signupClick(const QString &username, const QString &password, const QString &email,
                     const QString &voucherCode,  const QString &referringUsername);
    void heightChanged();

private slots:
    void onContinueClick();
    void onLanguageChanged();

    void onStandardLoginClick();
    void onHashedLoginClick();

    void onVoucherCodeClick();
    void onReferredClick();

    void onGenerateHashCodeClick();
    void onUploadFileClick();

private:
    IconButton *minimizeButton_;
    IconButton *closeButton_;
    IconButton *backButton_;

    bool isHashedMode_ = false;
    CommonGraphics::TextButton *standardLoginButton_;
    CommonGraphics::TextButton *hashedLoginButton_;

    UsernamePasswordEntry *usernameEntry_;
    UsernamePasswordEntry *passwordEntry_;
    CommonGraphics::TextItem *passwordHint_;
    UsernamePasswordEntry *emailEntry_;
    CommonGraphics::TextItem *emailHint_;

    UsernamePasswordEntry *hashEntry_;
    CommonGraphics::TextItem *hashHint_;

    CommonGraphics::TextWrapIconButton *btnVoucherCode_;
    bool isVoucherCodeExpanded_ = false;
    UsernamePasswordEntry *voucherEntry_;

    CommonGraphics::TextWrapIconButton *btnReferred_;
    bool isReferredExpanded_ = false;
    UsernamePasswordEntry *referredEntry_;

    CommonGraphics::TextItem *errorHint_;
    bool isError_ = false;
    QString curErrorText_;

    CommonGraphics::BubbleButton *continueButton_;

    int curHeight_;

    static constexpr int HEADER_HEIGHT               = 71;
    static constexpr int LOGIN_TEXT_HEIGHT    = 32;
    static constexpr int USERNAME_POS_Y_VISIBLE = 92;
    static constexpr int PASSWORD_POS_Y_VISIBLE = 164;


    void updatePositions();
    int centeredOffset(int background_length, int graphic_length);
    void showEditBoxes();

    // return true if everything ok
    bool verifyInputs();
    bool isPasswordValid(const QString &password);
    bool isEmailValid(const QString &email);

};

} // namespace LoginWindow
